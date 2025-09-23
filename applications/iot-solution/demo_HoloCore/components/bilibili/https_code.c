/**
 * @file https_code.c
 * @author Seahi-Mo (seahi-mo@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2025-09-22
 *
 * @copyright Ai-Thinker co.,ltd (c) 2025
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/tcp.h>
#include <lwip/err.h>
#include <http_client.h>
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/md5.h"
#include "mbedtls/debug.h"
#include <blog.h>
#include "https_code.h"
/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "api.bilibili.com"
#define WEB_PORT "443"
#define WEB_URL "https://api.bilibili.com/x/relation/stat?vmid=%s"

static const char *REQUEST = "GET " WEB_URL " HTTP/1.1\r\n"
							 "Host: " WEB_SERVER "\r\n"
							 "User-Agent: Ai-WB2 HoloCore \r\n"
							 "\r\n";

static char http_data[512];
static const uint8_t TEST_CERTIFICATE_FILENAME[] = {"-----BEGIN CERTIFICATE-----\r\n"
													"MIIEkjCCA3qgAwIBAgIQCgFBQgAAAVOFc2oLheynCDANBgkqhkiG9w0BAQsFADA/\r\n"
													"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\r\n"
													"DkRTVCBSb290IENBIFgzMB4XDTE2MDMxNzE2NDA0NloXDTIxMDMxNzE2NDA0Nlow\r\n"
													"SjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUxldCdzIEVuY3J5cHQxIzAhBgNVBAMT\r\n"
													"GkxldCdzIEVuY3J5cHQgQXV0aG9yaXR5IFgzMIIBIjANBgkqhkiG9w0BAQEFAAOC\r\n"
													"AQ8AMIIBCgKCAQEAnNMM8FrlLke3cl03g7NoYzDq1zUmGSXhvb418XCSL7e4S0EF\r\n"
													"q6meNQhY7LEqxGiHC6PjdeTm86dicbp5gWAf15Gan/PQeGdxyGkOlZHP/uaZ6WA8\r\n"
													"SMx+yk13EiSdRxta67nsHjcAHJyse6cF6s5K671B5TaYucv9bTyWaN8jKkKQDIZ0\r\n"
													"Z8h/pZq4UmEUEz9l6YKHy9v6Dlb2honzhT+Xhq+w3Brvaw2VFn3EK6BlspkENnWA\r\n"
													"a6xK8xuQSXgvopZPKiAlKQTGdMDQMc2PMTiVFrqoM7hD8bEfwzB/onkxEz0tNvjj\r\n"
													"/PIzark5McWvxI0NHWQWM6r6hCm21AvA2H3DkwIDAQABo4IBfTCCAXkwEgYDVR0T\r\n"
													"AQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwfwYIKwYBBQUHAQEEczBxMDIG\r\n"
													"CCsGAQUFBzABhiZodHRwOi8vaXNyZy50cnVzdGlkLm9jc3AuaWRlbnRydXN0LmNv\r\n"
													"bTA7BggrBgEFBQcwAoYvaHR0cDovL2FwcHMuaWRlbnRydXN0LmNvbS9yb290cy9k\r\n"
													"c3Ryb290Y2F4My5wN2MwHwYDVR0jBBgwFoAUxKexpHsscfrb4UuQdf/EFWCFiRAw\r\n"
													"VAYDVR0gBE0wSzAIBgZngQwBAgEwPwYLKwYBBAGC3xMBAQEwMDAuBggrBgEFBQcC\r\n"
													"ARYiaHR0cDovL2Nwcy5yb290LXgxLmxldHNlbmNyeXB0Lm9yZzA8BgNVHR8ENTAz\r\n"
													"MDGgL6AthitodHRwOi8vY3JsLmlkZW50cnVzdC5jb20vRFNUUk9PVENBWDNDUkwu\r\n"
													"Y3JsMB0GA1UdDgQWBBSoSmpjBH3duubRObemRWXv86jsoTANBgkqhkiG9w0BAQsF\r\n"
													"AAOCAQEA3TPXEfNjWDjdGBX7CVW+dla5cEilaUcne8IkCJLxWh9KEik3JHRRHGJo\r\n"
													"uM2VcGfl96S8TihRzZvoroed6ti6WqEBmtzw3Wodatg+VyOeph4EYpr/1wXKtx8/\r\n"
													"wApIvJSwtmVi4MFU5aMqrSDE6ea73Mj2tcMyo5jMd6jmeWUHK8so/joWUoHOUgwu\r\n"
													"X4Po1QYz+3dszkDqMp4fklxBwXRsW10KXzPMTZ+sOPAveyxindmjkW8lGy+QsRlG\r\n"
													"PfZ+G6Z6h7mjem0Y+iWlkYcV4PIWL1iwBi8saCbGS5jN2p8M+X+Q7UNKEkROb3N6\r\n"
													"KOqkqm57TH2H3eDJAkSnh6/DNFu0Qg==\r\n"
													"-----END CERTIFICATE-----\r\n"};
static const char *extract_json_from_http_response(const char *response);
char *https_get_code(char *user_id)
{

	int ret, flags, len;
	char buf[1024] = {0};
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_x509_crt cacert;
	mbedtls_ssl_config conf;
	mbedtls_net_context server_fd;

	mbedtls_ssl_init(&ssl);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_ctr_drbg_init(&ctr_drbg);
	blog_info("Seeding the random number generator");

	mbedtls_ssl_config_init(&conf);

	mbedtls_entropy_init(&entropy);
	if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
									 NULL, 0)) != 0)
	{
		blog_error("mbedtls_ctr_drbg_seed returned %d", ret);
		abort();
	}

	blog_info("Loading the CA root certificate...");
	ret = mbedtls_x509_crt_parse(&cacert,
								 TEST_CERTIFICATE_FILENAME,
								 strlen((char *)TEST_CERTIFICATE_FILENAME) + 1);

	if (ret < 0)
	{
		blog_error("mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
		abort();
	}

	blog_info("Setting hostname for TLS session...");

	/* Hostname set here should match CN in server certificate */
	if ((ret = mbedtls_ssl_set_hostname(&ssl, WEB_SERVER)) != 0)
	{
		blog_error("mbedtls_ssl_set_hostname returned -0x%x", -ret);
		abort();
	}

	blog_info("Setting up the SSL/TLS structure...");

	if ((ret = mbedtls_ssl_config_defaults(&conf,
										   MBEDTLS_SSL_IS_CLIENT,
										   MBEDTLS_SSL_TRANSPORT_STREAM,
										   MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
	{
		blog_error("mbedtls_ssl_config_defaults returned %d", ret);
		goto exit;
	}

	/* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
	   a warning if CA verification fails but it will continue to connect.

	   You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
	*/
	// mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE); // 跳过证书验证
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

	if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
	{
		blog_error("mbedtls_ssl_setup returned -0x%x\n\n", -ret);
		goto exit;
	}
	mbedtls_net_init(&server_fd);

	blog_info("Connecting to %s:%s...", WEB_SERVER, WEB_PORT);

	if ((ret = mbedtls_net_connect(&server_fd, WEB_SERVER,
								   WEB_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
	{
		blog_error("mbedtls_net_connect returned -%x", -ret);
		goto exit;
	}

	blog_info("Connected.");

	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

	blog_info("Performing the SSL/TLS handshake...");

	while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
	{
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			blog_error("mbedtls_ssl_handshake returned -0x%x", -ret);
			goto exit;
		}
	}

	blog_info("Verifying peer X.509 certificate...");

	if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
	{
		/* In real life, we probably want to close connection if ret != 0 */
		blog_warn("Failed to verify peer certificate!");
		bzero(buf, sizeof(buf));
		mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
		blog_warn("verification info: %s", buf);
	}
	else
	{
		blog_info("Certificate verified.");
	}

	blog_info("Cipher suite is %s", mbedtls_ssl_get_ciphersuite(&ssl));

	blog_info("Writing HTTP request...");
	char request_buf[256];
	sprintf(request_buf, REQUEST, user_id);
	blog_info("Request:\n%s", request_buf);
	size_t written_bytes = 0;
	do
	{
		ret = mbedtls_ssl_write(&ssl,
								(const unsigned char *)request_buf + written_bytes,
								strlen(request_buf) - written_bytes);
		if (ret >= 0)
		{
			blog_info("%d bytes written", ret);
			written_bytes += ret;
		}
		else if (ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_WANT_READ)
		{
			blog_error("mbedtls_ssl_write returned -0x%x", -ret);
			goto exit;
		}
	} while (written_bytes < strlen(request_buf));

	blog_info("Reading HTTP response...");

	do
	{
		len = sizeof(buf) - 1;
		bzero(buf, sizeof(buf));
		ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);

		if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
			continue;

		if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
		{
			ret = 0;
			break;
		}
		else if (ret < 0)
		{
			blog_error("mbedtls_ssl_read returned -0x%x", -ret);
			break;
		}
		else if (ret == 0)
		{
			blog_info("connection closed");
			break;
		}
		len = ret;
		strcpy(http_data, extract_json_from_http_response(buf));
	} while (1);

	mbedtls_ssl_close_notify(&ssl);
exit:
	mbedtls_ssl_session_reset(&ssl);
	mbedtls_net_free(&server_fd);

	if (ret != 0)
	{
		mbedtls_strerror(ret, buf, 100);
		blog_error("Last error was: -0x%x - %s", -ret, buf);
	}
	static int request_count;
	blog_info("Completed %d requests", ++request_count);
	len = strlen(http_data);
	if (len > 0)
		return http_data;
	return NULL;
}

static const char *extract_json_from_http_response(const char *response)
{
	// 查找响应头和响应体的分隔符：两个连续的换行符
	const char *separator = "\n\n";
	const char *json_start = strstr(response, separator);

	if (json_start == NULL)
	{
		// 如果找不到"\n\n"，尝试查找"\r\n\r\n"（标准HTTP分隔符）
		separator = "\r\n\r\n";
		json_start = strstr(response, separator);
		if (json_start == NULL)
		{
			return NULL; // 未找到分隔符
		}
		// 跳过分隔符
		json_start += strlen(separator);
	}
	else
	{
		// 跳过分隔符
		json_start += strlen(separator);
	}

	return json_start;
}