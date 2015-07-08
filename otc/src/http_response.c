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

/*JDH #include "mfs_config.h" */
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "drp.h"
#include "xaxi_eyescan.h"
#include "safe_printf.h"

#include "es_controller.h"

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

	if (lwip_write(sd, buf, len) != len) {
		//xil_printf("error writing http POST response to socket\r\n");
		//xil_printf("http header = %s\r\n", buf);
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

void generate_ber_table(FILE * stream, int ch_to_display) {
	fprintf(stream, "<TABLE>\n");

	fprintf(stream, "<tr>"
			"<th>Channel #</th>"
			"<th>Error count</th>"
			"<th>Sample count</th>"
			"</tr>\n");

	int i;
	for (i = 0; i < 48; ++i) {
		// DEBUG (remove)
		if (i < 12)
			xaxi_eyescan_enable_channel((u32) i);
		//END DEBUG
		if (xaxi_eyescan_channel_active((u32) i)) {
			eye_scan * eye_struct = get_eye_scan_lane(i);
			eye_scan_pixel * eye_pixel = eye_struct->pixels;
			int error_count = eye_pixel->center_error;
			u16 samp = eye_pixel->sample_count;
			u8 prescale = eye_pixel->prescale;
			int sample_count = samp * 32 << (1 + prescale);
			if (i == ch_to_display)
				fprintf(stream, "<tr style=\"color: LimeGreen; font-style: italic;\">");
			else
				fprintf(stream, "<tr>");
			fprintf(stream, "<th>%d</th>"
					"<td>%d</td>"
					"<td>%d</td>"
					"</tr>\n",
					i, error_count, sample_count);
		}
	}

	fprintf(stream, "</TABLE>\n");
}

void generate_eyescan_table(FILE * stream, int ch) {
	if (!xaxi_eyescan_channel_active(ch)) {
		fprintf(stream, "<text style=\"color:red\">Channel not active</text>");
		return;
	}

	eye_scan * eslane = get_eye_scan_lane(ch);
	int i;
	for(i = 0; i < sizeof(eslane->pixels)/sizeof(eye_scan_pixel*); ++i)
		xil_printf("ho: %d, vo: %d\n", eslane->pixels[i].h_offset, eslane->pixels[i].v_offset);
}

int parse_channel(char* req) {
	// The channel to display is a URL parameter. Parse it out of the request
	char * param_prefix = "/?ch=";
	char * ptr = strstr(req, param_prefix) + strlen(param_prefix);
	return atoi(ptr);
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
	fprintf(stream, "<META HTTP-EQUIV=""refresh"" CONTENT=""10; URL=192.168.1.99"">\n");
	fprintf(stream, "<STYLE>\n");
	fprintf(stream, "table, th, td { border: 1px solid black; border-collapse: collapse; }\n");
	fprintf(stream, "th, td { padding: 4px; }\n");
	fprintf(stream, "</STYLE>\n");
	fprintf(stream, "</HEAD>\n");

	/* ***********************
	 * System status
	 * ***********************/
	char *pagefmt = "<BODY>\n<CENTER><B>Xilinx VC707 System Status (10 s refresh)</B></CENTER><BR>\n"
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
	 * Bit error rates
	 * *******************/
	//	eyescan_lock();
		fprintf(stream, "<CENTER><B>Eyescan Data</B></CENTER><BR>\n");
		int ch_to_display = parse_channel(req);
		generate_ber_table(stream, ch_to_display);
		fprintf(stream, "<br><br>\n");
		fprintf(stream, "<form action=\"\" method=\"get\"> View Channel: <input type=\"text\" size=3 name=\"ch\" value=\"%d\"> <input type=\"submit\" value=\"Submit\"><br>", ch_to_display);
		fprintf(stream, "<br><br>\n");
		generate_eyescan_table(stream, ch_to_display);
		fprintf(stream, "<HR>\n");
	//	eyescan_unlock();

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


//int do_http_get(int sd, char *req, int rlen)
//{
//	char *response;
//	size_t response_size;
//	FILE *stream = open_memstream(&response, &response_size);
//	int w;
//
//#ifndef USE_XILINX_ORIGINAL
//	/* write the http headers */
//	char http_header[1024];
//	generate_http_header(http_header, "html", 0 /*strlen(pbuf)*/);
//	fprintf(stream, "%s\n", http_header);
//
//	/* now write the web page data in two steps.  FIrst the Xilinx temp/voltages */
//	char *pagefmt = " <HTML><HEAD><META HTTP-EQUIV=""refresh"" CONTENT=""10; URL=192.168.1.99""></HEAD"
//			"<BODY><CENTER><B>Xilinx VC707 System Status (10 s refresh)</B></CENTER><BR><HR>"
//			"Uptime: %d s<BR>"
//			"Temperature = %0.1f C<BR>"
//			"INT Voltage = %0.1f V<BR>"
//			"AUX Voltage = %0.1f V<BR>"
//			"BRAM Voltage = %0.1f V<BR><HR>";
//	fprintf(stream,pagefmt,procStatus.uptime,procStatus.v7temp,procStatus.v7vCCINT,procStatus.v7vCCAUX,procStatus.v7vBRAM);
//
//#if POLL_UPOD_TEMPS
//	char * upod_pagefmt = " <CENTER><B>uPod number %d at I2C Address %p</B></CENTER><BR><HR>"
//			"Status = 0x%02x<BR>"
//			"Temperature %d.%03d C<BR>"
//			"3.3V = %d uV<BR>"
//			"2.5V = %d uV<BR><HR>";
//
//	int idx = 0;
//	for( idx=0 ; idx<8 ; idx++ ) {
//
//		fprintf( stream , upod_pagefmt , idx , upod_address(idx) , upodStatus[idx]->status, \
//				upodStatus[idx]->tempWhole, upodStatus[idx]->tempFrac, \
//				100*upodStatus[idx]->v33, 100*upodStatus[idx]->v25 );
//	}
//#endif
//
//
//// Display error counts in each channel
//	char * ber_pagefmt = " <CENTER><B>GTX number %d</B></CENTER><BR><HR>"
//			"Error count = %d <BR>"
//			"Sample count = %d <BR><HR>";
//	int i;
//
//	eyescan_lock();
//	//    for (i = 0; i < 48; ++i) {
//	for (i = 0; i < 12; ++i) {
//		xaxi_eyescan_enable_channel((u32) i); // for debugging
//		if (xaxi_eyescan_channel_active((u32) i)){
//			eye_scan * eye_struct = get_eye_scan_lane(i);
//			eye_scan_pixel * eye_pixel = eye_struct->pixels;
//			u8 error_count = eye_pixel->center_error;
//			u16 samp = eye_pixel->sample_count;
//			u8 prescale = eye_pixel->prescale;
//			u32 sample_count = samp * 32 << (1 + prescale);
//			fprintf(stream, ber_pagefmt, i, (int) error_count, (int) sample_count);
//		}
//	}
//	eyescan_unlock();
//
//
//
//#ifdef THE_ETHERNET_COUNTERS_HAVE_BEEN_FIXED
//	/* Then the ethernet info */
//	pagefmt = "<BR>Ethernet information:<BR>"
//			"Rx bytes count = %d<BR>"
//			"Tx bytes count = %d<BR><HR>";
//	fprintf(stream,pagefmt,ethStatus.regVal[XAE_RXBL_OFFSET],ethStatus.regVal[XAE_TXBL_OFFSET]);
//#endif
//
//	/* and finally the end of the page */
//	fprintf(stream,"</BODY></HTML>");
//
//	/* flush the stream into the actual string (response) */
//	fflush(stream);
//
//	if ((w = lwip_write(sd, response, response_size)) < 0 ) {
//		xil_printf("error writing web page to socket\r\n");
//		xil_printf("attempted to lwip_write %d bytes, actual bytes written = %d\r\n", response_size, w);
//		return -4;
//	}
//
//	/* close the stream and deallocate the string */
//	fclose(stream);
//	free(response);
//
//	return 0;
//#else
//	/* determine file name */
//	extract_file_name(filename, req, rlen, MAX_FILENAME);
//
//	/* respond with 404 if not present */
//	if (mfs_exists_file(filename) == 0) {
//		//xil_printf("requested file %s not found, returning 404\r\n", filename);
//		do_404(sd, req, rlen);
//		return -1;
//	}
//
//	/* respond with correct file */
//
//	/* debug statement on UART */
//	//xil_printf("http GET: %s\r\n", filename);
//
//	/* get a pointer to file extension */
//	fext = get_file_extension(filename);
//
//	fd = mfs_file_open(filename, MFS_MODE_READ);
//
//	/* obtain file size,
//	 * note that lseek with offset 0, MFS_SEEK_END does not move file pointer */
//	fsize = mfs_file_lseek(fd, 0, MFS_SEEK_END);
//
//	/* write the http headers */
//	hlen = generate_http_header(buf, fext, fsize);
//	if (lwip_write(sd, buf, hlen) != hlen) {
//		//xil_printf("error writing http header to socket\r\n");
//		//xil_printf("http header = %s\r\n", buf);
//		return -1;
//	}
//
//	/* now write the file */
//	while (fsize) {
//		int w;
//		n = mfs_file_read(fd, buf, BUFSIZE);
//
//		if ((w = lwip_write(sd, buf, n)) != n) {
//			//xil_printf("error writing file (%s) to socket, remaining unwritten bytes = %d\r\n",
//			//filename, fsize - n);
//			//xil_printf("attempted to lwip_write %d bytes, actual bytes written = %d\r\n", n, w);
//			break;
//		}
//
//		fsize -= n;
//	}
//
//	mfs_file_close(fd);
//
//	return 0;
//#endif
//}

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
