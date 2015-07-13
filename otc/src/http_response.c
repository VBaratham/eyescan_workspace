/*
 * Copyright (c) 2007-2009 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <string.h>
#include <math.h>

/*JDH #include "mfs_config.h" */
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "drp.h"
#include "xaxi_eyescan.h"
#include "safe_printf.h"

#include "es_controller.h"
#include "es_simple_eye_acq.h"

#include "webserver.h"
/* JDH #include "platform_gpio.h" */
/* JDH #include "platform_fs.h" */

#include "SysStatus.h"
#include "otcLib/uPod.h"
#include "memstream.h"

char *notfound_header =
		"<html> \
    <head> \
        <title>404</title> \
        <style type=\"text/css\"> \
        div#request {background: #eeeeee} \
        </style> \
    </head> \
    <body> \
    <h1>404 Page Not Found</h1> \
    <div id=\"request\">";

char *notfound_footer =
		"</div> \
    </body> \
    </html>";


/* dynamically generate 404 response:
 *  this inserts the original request string in betwween the notfound_header & footer strings
 */
int do_404(int sd, char *req, int rlen)
{
	int len, hlen;
	int BUFSIZE = 1024;
	char buf[BUFSIZE];

	len = strlen(notfound_header) + strlen(notfound_footer) + rlen;

	hlen = generate_http_header(buf, "html", len);
	if (lwip_write(sd, buf, hlen) != hlen) {
		xil_printf("error writing http header to socket\r\n");
		xil_printf("http header = %s\n\r", buf);
		return -1;
	}

	lwip_write(sd, notfound_header, strlen(notfound_header));
	lwip_write(sd, req, rlen);
	lwip_write(sd, notfound_footer, strlen(notfound_footer));

	return 0;
}

int do_http_post(int sd, char *req, int rlen)
{
	int BUFSIZE = 1024;
	char buf[BUFSIZE];
	int len = 0, n;
	char *p;

	// Parse the number out of, "err_inj_ch_3=Inject+error(s)" (eg), which is part of the POST request body
	char * param_prefix = "err_inj_ch_";
	char * ptr = strstr(req, param_prefix) + strlen(param_prefix);
	int ch = atoi(ptr);

	xaxi_eyescan_error_inject(ch);

	// return redirect response
	char * response = "<html><head></head><body><script>window.location = document.referrer</script>";

	if (lwip_write(sd, response, strlen(response)) != strlen(response)) {
		xil_printf("error writing http POST response to socket\r\n");
		xil_printf("http header = %s\r\n", buf);
		return -1;
	}

	return 0;
}


void generate_upod_table(FILE * stream) {
	fprintf(stream, "<TABLE>\n");

	fprintf(stream, "<tr>"
			"<th>uPod #</th>"
			"<th>I2C addr</th>"
			"<th>Status</th>"
			"<th>Temperature</th>"
			"<th>3.3 V</th>"
			"<th>2.5 V</th>"
			"</tr>\n");

	int i;
	for (i = 0; i < 8; ++i)
		fprintf(stream, "<tr>"
				"<th>%d</th>"
				"<td>%p</td>"
				"<td>0x%02x</td>"
				"<td>%d.%03d C</td>"
				"<td>%d uV</td>"
				"<td>%d uV</td>"
				"</tr>\n",
				i, upod_address(i), upodStatus[i]->status, upodStatus[i]->tempWhole,
				upodStatus[i]->tempFrac, 100*upodStatus[i]->v33,
				100*upodStatus[i]->v25);

	fprintf(stream, "</TABLE>\n");
}

void generate_central_ber_table(FILE * stream, int ch_to_display) {
	if(pixel_ber_tables == NULL) {
		fprintf(stream, "<text style=\"color:red\">No BER data yet</text>");
		return;
	}

	fprintf(stream, "<TABLE>\n");

	fprintf(stream, "<tr>"
			"<th>Channel #</th>"
			"<th>Error count</th>"
			"<th>Sample count</th>"
			"</tr>\n");

	int i;
	for (i = 0; i < 48; ++i) {
		if (xaxi_eyescan_channel_active((u32) i)) {
			if (i == ch_to_display)
				fprintf(stream, "<tr style=\"color: LimeGreen; font-style: italic;\">");
			else
				fprintf(stream, "<tr>");
			fprintf(stream, "<th>%d</th>"
					"<td>%d</td>"
					"<td>%.3e</td>"
					"<td><form method=\"POST\"> <input type=\"submit\" name=\"err_inj_ch_%d\" value=\"Inject error(s)\"> </input> </form>"
					"<button onclick=\"view_channel(%d)\">View channel</button></td>"
					"</tr>\n",
					i, central_err_cnts[i], central_samp_cnts[i], i, i);
		}
		//		else {
		//			fprintf(stream, "<tr style=\"color: Red;\">");
		//			fprintf(stream, "<td>%d (inactive)</td></tr>\n", i);
		//		}
	}

	fprintf(stream, "</TABLE>\n");
	fprintf(stream, "<p>Note: Error injection works about 75%% of the time</p>\n");
	fprintf(stream, "<p>Note: Disabled channels not shown</p>\n");
	fprintf(stream, "<br><br>\n");
	if(ch_to_display == -1)
		fprintf(stream, "View channel (or \"none\"): <input type=\"text\" size=3 id=\"view_ch\" value=\"none\" onkeydown=\"check_enter(event)\"> <button type=\"submit\" onclick=\"view_channel_from_form()\">Go</button><br>");
	else
		fprintf(stream, "View channel (or \"none\"): <input type=\"text\" size=3 id=\"view_ch\" value=\"%d\" onkeydown=\"check_enter(event)\"> <button type=\"submit\" onclick=\"view_channel_from_form()\">Go</button><br>", ch_to_display);
}

void generate_eyescan_table(FILE * stream, int ch) {
	if(pixel_ber_tables == NULL) {
		fprintf(stream, "<text style=\"color:red\">No eyescan data yet</text>");
		return;
	}

	if (ch == -1){
		fprintf(stream, "<text style=\"color:red\">Select a channel to view eyescan data</text>");
		return;
	}

	if (!xaxi_eyescan_channel_active(ch)) {
		fprintf(stream, "<text style=\"color:red\">Channel not active</text>");
		return;
	}

	fprintf(stream, "Channel %d eyescan data:<br>\n", ch);

	eye_scan * eye_struct = get_eye_scan_lane(ch);

	// Write the table by looping over the stored tables from es_simple_eye_acq
	fprintf(stream, "<TABLE>\n");
	u16 max_vert_offset = get_max_vert_offset(eye_struct->vert_step_size);
	int row, col;
	int horz_size = 2 * (eye_struct->max_horz_offset / eye_struct->horz_step_size) + 1;
	int vert_size = 2 * (max_vert_offset / eye_struct->vert_step_size) + 1;
	for(row = 0; row < vert_size; ++row) {
		fprintf(stream, "<tr>\n");
		for(col = 0; col < horz_size; ++col){
			double ber = pixel_ber_tables[ch][row * horz_size + col];
			double MAX_BER = .001; // Anything worse than this renders as red. Anything only slightly better than this (4-5 orders of magnitude) renders as red/orange/yellow.

			u32 color;
			if (ber > MAX_BER)
				color = 0;
			else
				color = 120 * (1 - log(MAX_BER) / log(ber));

			if(ber != ber) // This is the preferred way to check if something is NAN, for some reason.
				fprintf(stream, "<td></td>\n");
			else
				fprintf(stream, "<td style=\"color:hsl(%d, 100%%, 50%%); font-size:12px;\">%.2e <br> <text style=\"font-size:9px\"> (%d, %d) </text> </td>\n", color, ber, row, col);
		}
		fprintf(stream, "</tr>\n");
	}
	fprintf(stream, "</TABLE>\n");

	// DEBUG
	// Print the center error, and sample count from each pixel.
//	int i;
//	for(i = 0; i < eye_struct->pixel_count; ++i){
//		fprintf(stream, "center_error: %d, sample_count: %d<br>\n", eye_struct->pixels[i].center_error, eye_struct->pixels[i].sample_count);
//	}
}

int parse_channel(char* req) {
	// The channel to display is a URL parameter. Parse it out of the request
	char * param_prefix = "GET /";
	char * ptr = strstr(req, param_prefix) + strlen(param_prefix);
	if(isdigit(*ptr))
		return atoi(ptr);
	else return -1;
}

/* respond for a file GET request */
int do_http_get(int sd, char *req, int rlen) {
	char *response;
	size_t response_size;
	FILE *stream = open_memstream(&response, &response_size);

	/* *****************
	 *  HTTP header
	 * *****************/
	char http_header[1024]; // Chose this size randomly, it's more than enough
	generate_http_header(http_header, "html", 0);
	fprintf(stream, "%s", http_header);

	/* *****************
	 * HTML head
	 * *****************/
	fprintf(stream, "<HTML>\n");
	fprintf(stream, "<HEAD>\n");
//	fprintf(stream, "<META HTTP-EQUIV=""refresh"" CONTENT=""10; URL=192.168.1.99"">\n");
	fprintf(stream, "<STYLE>\n");
	fprintf(stream, "table, th, td { border: 1px solid black; border-collapse: collapse; }\n");
	fprintf(stream, "th, td { padding: 4px; }\n");
	fprintf(stream, "</STYLE>\n");
	fprintf(stream, "<SCRIPT>function check_enter(e){ if(e.keyCode == 13) {view_channel_from_form();} }</SCRIPT>\n");
	fprintf(stream, "<SCRIPT>function view_channel_from_form(){ window.location = document.getElementById(\"view_ch\").value; }</SCRIPT>\n");
	fprintf(stream, "<SCRIPT>function view_channel(ch){ window.location = ch; }</SCRIPT>\n");
	fprintf(stream, "</HEAD>\n");

	/* ***********************
	 * System status
	 * ***********************/
	char *pagefmt = "<BODY>\n<CENTER><B>Xilinx VC707 System Status</B></CENTER><BR>\n"
			"Uptime: %d s<BR>"
			"Temperature = %0.1f C<BR>\n"
			"INT Voltage = %0.1f V<BR>\n"
			"AUX Voltage = %0.1f V<BR>\n"
			"BRAM Voltage = %0.1f V<BR>\n<HR>";
	fprintf(stream,pagefmt,procStatus.uptime,procStatus.v7temp,procStatus.v7vCCINT,procStatus.v7vCCAUX,procStatus.v7vBRAM);

#ifdef POLL_UPOD_TEMPS
	/* *******************
	 * uPod status
	 * *******************/
	fprintf(stream, "<CENTER><B>uPod Status</B></CENTER><BR>\n");
	generate_upod_table(stream);
	fprintf(stream, "<HR>\n");
#endif

	/* *******************
	 * Eyescan data
	 * *******************/
	fprintf(stream, "<CENTER><B>Eyescan Data</B></CENTER><BR>\n");
	int ch_to_display = parse_channel(req);
	generate_central_ber_table(stream, ch_to_display);
	fprintf(stream, "<br><br>\n");
	generate_eyescan_table(stream, ch_to_display);
	fprintf(stream, "<HR>\n");

	/* ***********************
	 * Close HTML, send data
	 * ***********************/
	fprintf(stream,"</BODY>\n</HTML>\n");
	fflush(stream);
	//	xil_printf("%s\n", response); //DEBUG
	int w;
	if ((w = lwip_write(sd, response, response_size)) < 0 ) {
		xil_printf("error writing web page to socket\r\n");
		xil_printf("attempted to lwip_write %d bytes, actual bytes written = %d\r\n", response_size, w);
		return -4;
	}
	fclose(stream);
	free(response);

	return 0;
}


enum http_req_type { HTTP_GET, HTTP_POST, HTTP_UNKNOWN };
enum http_req_type decode_http_request(char *req, int l)
{
	char *get_str = "GET";
	char *post_str = "POST";

	if (!strncmp(req, get_str, strlen(get_str)))
		return HTTP_GET;

	if (!strncmp(req, post_str, strlen(post_str)))
		return HTTP_POST;

	return HTTP_UNKNOWN;
}

/* generate and write out an appropriate response for the http request */
int generate_response(int sd, char *http_req, int http_req_len)
{
	enum http_req_type request_type = decode_http_request(http_req, http_req_len);

	switch(request_type) {
	case HTTP_GET:
		return do_http_get(sd, http_req, http_req_len);
	case HTTP_POST:
		return do_http_post(sd, http_req, http_req_len);
	default:
		return do_404(sd, http_req, http_req_len);
	}
}
