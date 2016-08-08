#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>

#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_errno.h>

#include "config.h"

#define DIE(...) die(__FILE__, __func__, __LINE__, __VA_ARGS__)
#define WARN(...) info(__FILE__, __func__, __LINE__, __VA_ARGS__)

#if ENABLE_DEBUG
#define DEBUG(...) info(__FILE__, __func__, __LINE__, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

struct comm_context {
	struct fi_info *info;
	struct fid_fabric *fabric;
	struct fid_domain *domain;
	struct fid_av *av;
	struct fid_eq *eq;
};

/******************************************************************************
 * Logging related code
 *****************************************************************************/
void vlogf(const char *file, const char *func, int line, const char *fmt,
	   va_list vargs)
{
	char buf[8192] = {0};
	int size = 0;

	/* Log in the format:
	 *   [FILE]:FUNCTION():<LINE>: message
	 */
	size = snprintf(buf, sizeof(buf), "[%s]:%s():<%d> ", file, func, line);
	vsnprintf(buf + size, sizeof(buf) - size, fmt, vargs);

	fprintf(stderr, "%s", buf);
}

/* Log the given message.
 */
void info(const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list vargs;

	va_start(vargs, fmt);
	vlogf(file, func, line, fmt, vargs);
	va_end(vargs);
}

/* Log the message and exit with failure exit code.
 */
void die(const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogf(file, func, line, fmt, args);
	va_end(args);

	exit(EXIT_FAILURE);
}

/******************************************************************************
 * Libfabric resource management
 *****************************************************************************/
void setup_fabric(struct comm_context *context, const char *provider_name,
		  const char *fabric_name)
{
	struct fi_info *hints;
	int ret;

	hints = fi_allocinfo();
	if (!hints)
		DIE("unable to allocate space for hints\n");

	if (provider_name)
		hints->fabric_attr->prov_name = strdup(provider_name);
	if (fabric_name)
		hints->fabric_attr->name = strdup(fabric_name);

	DEBUG("provided provider name hint: %s\n",
	      hints->fabric_attr->prov_name);
	DEBUG("provided fabric name hint: %s\n", hints->fabric_attr->name);

	hints->ep_attr->type = FI_EP_DGRAM;

	hints->mode = FI_LOCAL_MR;
	hints->caps = FI_MSG;

	ret =
	    fi_getinfo(FI_VERSION(1, 1), NULL, NULL, 0, hints, &context->info);
	if (ret)
		DIE("fi_getinfo: %s\n", fi_strerror(-ret));

	fi_freeinfo(hints);

	ret = fi_fabric(context->info->fabric_attr, &context->fabric, NULL);
	if (ret)
		DIE("fi_fabric: %s\n", fi_strerror(-ret));

	DEBUG("opened fabric with provider: %s\n",
	      context->info->fabric_attr->prov_name);
	DEBUG("bound to: %s\n", context->info->fabric_attr->name);
}

void setup_resources(struct comm_context *context)
{
	int ret;

	struct fi_av_attr av_attr = {
		.type = FI_AV_MAP,
		.flags = FI_EVENT,
		.count = 8
	};

	struct fi_eq_attr eq_attr = {
		.size = 64,
		.wait_obj = FI_WAIT_UNSPEC
	};

	ret = fi_eq_open(context->fabric, &eq_attr, &context->eq, NULL);
	if (ret)
		DIE("fi_eq_open: %s\n", fi_strerror(-ret));

	ret = fi_domain(context->fabric, context->info, &context->domain, NULL);
	if (ret)
		DIE("fi_domain: %s\n", fi_strerror(-ret));

	ret = fi_av_open(context->domain, &av_attr, &context->av, NULL);
	if (ret)
		DIE("fi_av_open: %s\n", fi_strerror(-ret));

	ret = fi_av_bind(context->av, &context->eq->fid, 0);
	if (ret)
		DIE("fi_av_bind: %s\n", fi_strerror(-ret));
}

void free_resources(struct comm_context *context)
{
	fi_close(&context->av->fid);
	fi_close(&context->domain->fid);
	fi_close(&context->fabric->fid);
	fi_freeinfo(context->info);
}

/******************************************************************************
 * Actual test, try to insert address into opened AV and print address if
 * successful
 *****************************************************************************/
void handle_error(struct comm_context *context)
{
	struct fi_eq_err_entry err_entry;
	int ret;

	ret = fi_eq_readerr(context->eq, &err_entry, 0);
	if (ret < 0)
		DIE("encountered error while reading error queue\n");

	printf("Resolution failed: [%d]: <%s>\n", err_entry.err,
	       fi_strerror(err_entry.err));
}

void resolve_address(struct comm_context *context, const char *addr,
		     const char *port)
{
	struct fi_eq_entry entry;
	fi_addr_t fi_addr;
	uint32_t event;
	char resolved_addr[64] = {0};
	char buf[64] = {0};
	size_t size = 0;
	int ret;

	ret = fi_av_insertsvc(context->av, addr, port, &fi_addr, 0, NULL);
	if (ret)
		DIE("fi_av_insertsvc: %s\n", fi_strerror(-ret));

	ret = fi_eq_sread(context->eq, &event, &entry, sizeof(entry), -1, 0);
	if (ret < 0) {
		if (ret == -FI_EAVAIL) {
			handle_error(context);
			return;
		}

		DIE("fi_eq_sread: %s\n", fi_strerror(-ret));
	}

	if (event != FI_AV_COMPLETE)
		DIE("fi_av_complete: got event %d\n", event);

	/* After the insertion has completed, look it up and try to convert it
	 * back to string format to make sure all is cool.
	 */
	size = sizeof(resolved_addr);
	ret = fi_av_lookup(context->av, fi_addr, &resolved_addr, &size);
	if (ret)
		DIE("fi_av_lookup: %s\n", fi_strerror(-ret));

	size = sizeof(buf);
	fi_av_straddr(context->av, &resolved_addr, buf, &size);

	printf("Successfully resolved address to %s\n", buf);
}

void print_usage(const char *program_name, FILE *stream)
{
	fprintf(stream,
		"A simple libfabric application to resolve an address\n\n");
	fprintf(stream, "Usage:\t%s [options] <address>\n\n", program_name);
	fprintf(stream, "\t-f, <fabric name>\tfabric to bind to\n");
	fprintf(stream, "\t-p, <port>\t\tport to resolve (default: 1337)\n");
	fprintf(stream, "\t-P, <provider name>\tlibfabric provider to use\n");
	fprintf(stream, "\t-h,\t\t\tprint this help and exit\n");
}

int main(int argc, char **argv)
{
	struct comm_context context;
	const char *provider_name = NULL;
	const char *fabric_name = NULL;
	const char *address = NULL;

	/* If fi_av_insertsvc is given 0 as the port for usNIC, it will return
	 * -FI_EINVAL. Arbitrarily pick 1337 as the default port.
	 */
	const char *port = "1337";
	int c;

	/* Older versions of GCC seem to dislike the {0} notation. */
	memset(&context, 0, sizeof(context));

	while ((c = getopt(argc, argv, "hf:p:P:")) != -1) {
		switch (c) {
		case 'f':
			fabric_name = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'P':
			provider_name = optarg;
			break;
		case 'h':
			print_usage(argv[0], stdout);
			exit(EXIT_SUCCESS);
		case '?':
		default:
			print_usage(argv[0], stderr);
			exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		WARN("%s: need an address\n\n", argv[0]);
		print_usage(argv[0], stderr);
		exit(EXIT_FAILURE);
	}

	address = argv[optind];

	setup_fabric(&context, provider_name, fabric_name);
	setup_resources(&context);
	resolve_address(&context, address, port);
	free_resources(&context);

	return 0;
}
