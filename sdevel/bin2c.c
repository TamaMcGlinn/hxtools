/*
 *	bin2c - convert arbitrary files into C variable definitions
 *	Copyright by Jan Engelhardt, 2004-2008
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *	For details, see the file named "LICENSE.GPL2".
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libHX/ctype_helper.h>
#include <libHX/option.h>
#include <libHX/string.h>
#if CHAR_BIT > 8
#	error Sorry, we do not support CHAR_BIT > 8 yet.
#endif

static hxmc_t *btc_cfile, *btc_hfile;
static hxmc_t *btc_guard_name;
static unsigned int btc_verbose;
static const char btc_quote_needed[] = "\"?\\";

static size_t btc_qsize_cstring(const void *src, size_t input_size)
{
	const unsigned char *p = src;
	size_t z = 0;

	for (; input_size-- > 0; ++p)
		z += (!HX_isprint(*p) ||
		     strchr(btc_quote_needed, *p) != NULL) ?
		     4 /* "\xXY" */ : 1;
	return z;
}

static char *btc_memquote(const void *vsrc, size_t input_size)
{
	size_t quoted_size = btc_qsize_cstring(vsrc, input_size);
	char *out = malloc(quoted_size + 1), *p = out;
	const unsigned char *src = vsrc;

	if (out == NULL)
		abort();
	for (; input_size-- > 0; ++src) {
		if (!HX_isprint(*src) ||
		    strchr(btc_quote_needed, *src) != NULL) {
			*p++ = '\\';
			*p++ = '0' + ((*src & 0700) >> 6);
			*p++ = '0' + ((*src & 0070) >> 3);
			*p++ = '0' + (*src & 0007);
		} else {
			*p++ = *src;
		}
	}
	*p = '\0';
	return out;
}

static char *btc_strquote(const char *src)
{
	return btc_memquote(src, strlen(src));
}

/**
 * Output the binary stream in a "normal" fashion.
 */
static void btc_output_normal(FILE *cfilp, FILE *hfilp, FILE *ifilp,
    size_t isize, const char *vname)
{
	char input_buf[4096], *output_buf;
	size_t input_len;

	if (cfilp != hfilp) {
		fprintf(hfilp, "extern const unsigned char bin2c_%s[%zu];\n",
		        vname, isize);
		fprintf(cfilp, "const unsigned char bin2c_%s[%zu] = \"",
		        vname, isize);
	} else {
		fprintf(cfilp, "static const unsigned char bin2c_%s[%zu] = \"",
		        vname, isize);
	}

	while ((input_len = fread(input_buf, 1, sizeof(input_buf), ifilp)) > 0) {
		output_buf = btc_memquote(input_buf, input_len);
		if (output_buf == NULL)
			abort();
		fwrite(output_buf, strlen(output_buf), 1, cfilp);
		free(output_buf);
	}
	fprintf(cfilp, "\";\n");
}

/**
 * Construct a variable identifier from the given input filename.
 * Just substitute all non-alphanumeric characters by underscore.
 */
static hxmc_t *btc_construct_vname(const char *cfile)
{
	hxmc_t *vname = HXmc_strinit(cfile);
	char *p;

	if (vname == NULL)
		return NULL;
	if (!HX_isalpha(*cfile) && *cfile != '_') {
		/*
		 * The identifier must begin with [a-z_]; in any case, it
		 * cannot begin with a digit or any special character.
		 */
		if (HXmc_strpcat(&vname, "_") == NULL)
			/* can happen but unlikely: just abort w/o errormsg */
			abort();
		p = vname + 1;
	} else {
		p = vname;
	}
	for (; *cfile != '\0'; ++cfile)
		*p++ = HX_isalnum(*cfile) ? *cfile : '_';
	return vname;
}

/**
 * Process a single input file.
 */
static int btc_process_single(FILE *cfilp, FILE *hfilp, const char *ifile)
{
	struct stat sb;
	hxmc_t *vname;
	size_t input_len;
	FILE *ifilp;

	ifilp = fopen(ifile, "r");
	if (ifilp == NULL) {
		fprintf(stderr, "bin2c: ERROR: Could not open %s for "
		        "reading: %s\n", ifile, strerror(errno));
		return -errno;
	}

	fprintf(cfilp, "/* Autogenerated from %s */\n", ifile);
	if (stat(ifile, &sb) != 0) {
		fprintf(stderr, "ERROR: Cannot stat %s: %s\n", ifile,
		        ifile, strerror(errno));
		return -errno;
	}
	vname = btc_construct_vname(ifile);
	btc_output_normal(cfilp, hfilp, ifilp, sb.st_size, vname);
	HXmc_free(vname);
	fclose(ifilp);
	return 0;
}

/**
 * Process all files given in @argv.
 */
static int btc_start(const char **argv)
{
	FILE *hfilp, *cfilp = NULL;
	char *result;
	int ret = 0;

	hfilp = fopen(btc_hfile, "w");
	if (hfilp == NULL) {
		fprintf(stderr, "bin2c: ERROR: Could nt open %s for writing: "
		        "%s\n", btc_hfile, strerror(errno));
		return -errno;
	}

	fprintf(hfilp, "/* Autogenerated by hxtools bin2c */\n");

	if (btc_cfile != NULL) {
		cfilp = fopen(btc_cfile, "w");
		if (cfilp == NULL) {
			fprintf(stderr, "bin2c: ERROR: Could not open %s for "
			        "writing: %s\n", btc_cfile, strerror(errno));
			return -errno;
		}
		fprintf(cfilp, "/* Autogenerated by hxtools bin2c */\n");
		result = btc_strquote(btc_hfile);
		fprintf(cfilp, "#include \"%s\"\n", result);
		free(result);
	} else {
		cfilp = hfilp;
	}

	if (btc_guard_name)
		fprintf(hfilp, "#ifndef %s\n#define %s 1\n\n",
		        btc_guard_name, btc_guard_name);
	fprintf(hfilp, "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");

	while (*++argv != NULL) {
		ret = btc_process_single(cfilp, hfilp, *argv);
		if (ret != 0)
			break;
	}

	fprintf(hfilp, "\n#ifdef __cplusplus\n} /* extern \"C\" */\n#endif\n");
	if (btc_guard_name)
		fprintf(hfilp, "\n\n#endif /* %s */\n", btc_guard_name);
	fclose(hfilp);
	if (cfilp != hfilp)
		fclose(cfilp);
	return ret;
}

static const struct HXoption btc_option_table[] = {
	{.sh = 'C', .type = HXTYPE_MCSTR, .ptr = &btc_cfile,
	 .help = "Filename for the output .c file", .htyp = "FILE"},
	{.sh = 'G', .type = HXTYPE_MCSTR, .ptr = &btc_guard_name,
	 .help = "Name for the header's include guard"},
	{.sh = 'H', .type = HXTYPE_MCSTR, .ptr = &btc_hfile,
	 .help = "Filename for the output .h file", .htyp = "FILE"},
	{.sh = 'v', .type = HXTYPE_NONE, .ptr = &btc_verbose,
	 .help = "Be verbose during operation"},
	HXOPT_AUTOHELP,
	HXOPT_TABLEEND,
};

/**
 * Detect any C/non-C file naming and either return the corresponding
 * header file suffix, or report the non-C suffix problem.
 */
static const char *btc_known_suffix(const char *s)
{
	bool modern_cp, archaic_cp;

	if (strcmp(s, ".c") == 0)
		return ".h";

	modern_cp = strcmp(s, ".cpp") == 0 || strcmp(s, ".cxx") == 0;
	archaic_cp = strcmp(s, ".cc") == 0 || strcmp(s, ".C") == 0 ||
	             strcmp(s, ".cp") == 0 || strcmp(s, ".CPP") == 0;
	if (modern_cp || archaic_cp)
		fprintf(stderr, "bin2c: WARNING: bin2c outputs C code, not "
		        "C++ -- It is wrong to call the output file %s!\n", s);
	else
		fprintf(stderr, "bin2c: WARNING: The suffix %s is "
		        "unknown!\n", s);
	return NULL;
}

/**
 * Construct a filename from the given .c-ish filename.
 * Warn about C++ naming, because we are really generating a C program,
 * not a C++ program.
 */
static hxmc_t *btc_construct_hname(const char *cfile)
{
	hxmc_t *hfile = HXmc_strinit(cfile);
	const char *suffix, *repl;

	if (hfile == NULL)
		return NULL;
	suffix = strrchr(hfile, '.');
	if (suffix == NULL || (repl = btc_known_suffix(suffix)) == NULL) {
		/* No extension at all, or unknown suffix. */
		if (HXmc_strcat(&hfile, ".h") == NULL)
			goto out;
		return hfile;
	}

	/* We have a replacement… */
	if (HXmc_trunc(&hfile, suffix - hfile) == NULL)
		abort(); /* never to happen */
	if (HXmc_strcat(&hfile, repl) == NULL)
		goto out;
	return hfile;

 out:
	HXmc_free(hfile);
	return NULL;
}

/**
 * Construct an identifer name for the guarding #define from the filename.
 * Transformation: a-z -> A-Z, 0-9 -> (keep), * -> underscore.
 */
static hxmc_t *btc_construct_guard(const char *s)
{
	char *p, *guard = HXmc_strinit(s);

	if (guard == NULL)
		return NULL;
	if (!HX_isalpha(*s) && *s != '_') {
		/*
		 * The identifier must begin with [a-z_]; in any case, it
		 * cannot begin with a digit or any special character.
		 */
		if (HXmc_strpcat(&guard, "_") == NULL)
			/* can happen but unlikely: just abort w/o errormsg */
			abort();
		p = guard + 1;
	} else {
		p = guard;
	}

	for (; *s != '\0'; ++s)
		*p++ = HX_isalnum(*s) ? HX_toupper(*s) : '_';
	return guard;
}

/**
 * Parse options and fill in any defaults for the variables.
 */
static bool btc_get_options(int *argc, const char ***argv)
{
	int ret;

	ret = HX_getopt(btc_option_table, argc, argv, HXOPT_USAGEONERR);
	if (ret != HXOPT_ERR_SUCCESS)
		return false;
	if (btc_cfile != NULL && btc_hfile == NULL)
		btc_hfile = btc_construct_hname(btc_cfile);
	if (btc_hfile == NULL) {
		fprintf(stderr, "bin2c: you need to specify -C or -H, or both\n");
		return false;
	}
	if (btc_cfile != NULL && btc_guard_name == NULL)
		btc_guard_name = btc_construct_guard(btc_hfile);
	if (btc_verbose) {
		printf("C program file: %s\n",
		       (btc_cfile != NULL) ? btc_cfile : "");
		printf("C header file: %s\n", btc_hfile);
		printf("Header guard name: %s\n",
		       (btc_guard_name != NULL) ? btc_guard_name : "");
	}
	return true;
}

int main(int argc, const char **argv)
{
	int ret;

	if (!btc_get_options(&argc, &argv))
		return EXIT_FAILURE;
	ret = btc_start(argv);
	HXmc_free(btc_cfile);
	HXmc_free(btc_hfile);
	HXmc_free(btc_guard_name);
	return (ret == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
