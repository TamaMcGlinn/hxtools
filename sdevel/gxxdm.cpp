/*
 *	An educational piece-by-piece G++ symbol name demangler
 *	in the style of wireshark's bytewise highlighting
 *	Copyright by Jan Engelhardt, 2016
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *	For details, see the file named "LICENSE.GPL2".
 */
#include <list>
#include <string>
#include <stdio.h>
#include <libHX/string.h>

struct seg {
	size_t start, end;
	std::string desc;
	std::list<seg> sub;
};

static const char c_hi[] = "\e\1331;37;42m";
static const char c_normal[] = "\e\1330m";

static ssize_t dparse_type(std::list<seg> &, const char *, size_t);
static ssize_t dparse_paramlist(std::list<seg> &, const char *, size_t, char = '\0');

static ssize_t dparse_digitname(std::list<seg> &ast, const char *sym, size_t idx)
{
	size_t ndigit = strspn(sym + idx, "0123456789");
	size_t nchar = 0;
	size_t i = ndigit;

	for (const char *p = sym + idx; i-- > 0; ++p) {
		nchar *= 10;
		nchar += *p - '0';
	}
	ast.push_back({idx, idx + ndigit, "length of name"});
	if (strnlen(sym + idx, ndigit + nchar) < ndigit + nchar) {
		/* not enough chars before end of string */
		ast.push_back({idx + ndigit, idx + ndigit + nchar, "name (incomplete)"});
		return -1;
	}
	ast.push_back({idx + ndigit, idx + ndigit + nchar, "name"});
	return ast.rbegin()->end;
}

static ssize_t dparse_nspace(std::list<seg> &ast, const char *sym, size_t idx)
{
	ast.push_back({idx, idx, "nspace"});
	if (sym[idx] != 'N')
		return -1;
	++idx;
	auto &tl = *ast.rbegin();
	while (isdigit(sym[idx])) {
		int ret = dparse_digitname(tl.sub, sym, idx);
		if (ret < 0)
			return ret;
		idx = ret;
	}
	tl.end = idx;
	if (sym[idx] != 'E')
		return -1;
	tl.end = ++idx;
	return idx;
}

static ssize_t dparse_functype(std::list<seg> &ast, const char *sym, size_t idx)
{
	ast.push_back({idx, idx, "return type"});
	auto &rt_type = *ast.rbegin();
	int ret = dparse_type(rt_type.sub, sym, idx);
	if (ret < 0)
		return -1;
	rt_type.end = idx = ret;

	ret = dparse_paramlist(ast, sym, idx, 'E');
	if (ret < 0)
		return -1;
	ast.rbegin()->end = idx = ret;
	return idx;
}

static ssize_t dparse_type(std::list<seg> &ast, const char *sym, size_t idx)
{
	if (sym[idx] == 'r') ast.push_back({idx, idx + 1, "restrict"});
	else if (sym[idx] == 'F') ast.push_back({idx, idx + 1, "function"});
	else if (sym[idx] == 'K') ast.push_back({idx, idx + 1, "const"});
	else if (sym[idx] == 'P') ast.push_back({idx, idx + 1, "pointer to"});
	else if (sym[idx] == 'R') ast.push_back({idx, idx + 1, "reference to"});
	else if (sym[idx] == 'O') ast.push_back({idx, idx + 1, "uref to"});
	else if (sym[idx] == 'V') ast.push_back({idx, idx + 1, "volatile"});
	if (strchr("KOPRVr", sym[idx]) != NULL) {
		auto &ptr = *ast.rbegin();
		if (sym[idx+1] == '\0')
			return -1;
		int ret = dparse_type(ptr.sub, sym, idx + 1);
		if (ret < 0)
			return ret;
		idx = ret;
		return idx;
	}
	else if (sym[idx] == 'F') {
		auto &ptr = *ast.rbegin();
		if (sym[idx+1] == '\0')
			return -1;
		int ret = dparse_functype(ptr.sub, sym, idx + 1);
		if (ret < 0)
			return ret;
		if (sym[ret] == 'E')
			++ret;
		ptr.end = ret;
		return ret;
	}
	else if (isdigit(sym[idx])) {
		int ret = dparse_digitname(ast, sym, idx);
		if (ret < 0)
			return ret;
		idx = ret;
		return idx;
	}
	else if (sym[idx] == 'N') {
		int ret = dparse_nspace(ast, sym, idx);
		if (ret < 0)
			return ret;
		idx = ret;
		return idx;
	}
	else if (sym[idx] == 'D' && sym[idx+1] == 'a') { ast.push_back({idx, idx + 1, "auto"}); }
	else if (sym[idx] == 'D' && sym[idx+1] == 'd') { ast.push_back({idx, idx + 1, "decimal64"}); }
	else if (sym[idx] == 'D' && sym[idx+1] == 'e') { ast.push_back({idx, idx + 1, "decimal128"}); }
	else if (sym[idx] == 'D' && sym[idx+1] == 'f') { ast.push_back({idx, idx + 1, "decimal32"}); }
	else if (sym[idx] == 'D' && sym[idx+1] == 'h') { ast.push_back({idx, idx + 1, "half"}); }
 	else if (sym[idx] == 'D' && sym[idx+1] == 'i') { ast.push_back({idx, idx + 1, "char32_t"}); }
	else if (sym[idx] == 'D' && sym[idx+1] == 'n') { ast.push_back({idx, idx + 1, "decltype(nullptr)"}); }
	else if (sym[idx] == 'D' && sym[idx+1] == 's') { ast.push_back({idx, idx + 1, "char16_t"}); }
	else if (sym[idx] == 'a') { ast.push_back({idx, idx + 1, "signed char"}); }
	else if (sym[idx] == 'b') { ast.push_back({idx, idx + 1, "bool"}); }
	else if (sym[idx] == 'c') { ast.push_back({idx, idx + 1, "char"}); }
	else if (sym[idx] == 'd') { ast.push_back({idx, idx + 1, "double"}); }
	else if (sym[idx] == 'e') { ast.push_back({idx, idx + 1, "long double"}); }
	else if (sym[idx] == 'f') { ast.push_back({idx, idx + 1, "float"}); }
	else if (sym[idx] == 'g') { ast.push_back({idx, idx + 1, "__float128"}); }
	else if (sym[idx] == 'h') { ast.push_back({idx, idx + 1, "unsigned char"}); }
	else if (sym[idx] == 'i') { ast.push_back({idx, idx + 1, "int"}); }
	else if (sym[idx] == 'j') { ast.push_back({idx, idx + 1, "unsigned int"}); }
	else if (sym[idx] == 'l') { ast.push_back({idx, idx + 1, "long"}); }
	else if (sym[idx] == 'm') { ast.push_back({idx, idx + 1, "unsigned long"}); }
	else if (sym[idx] == 'n') { ast.push_back({idx, idx + 1, "__int128"}); }
	else if (sym[idx] == 'o') { ast.push_back({idx, idx + 1, "unsigned __int128"}); }
	else if (sym[idx] == 's') { ast.push_back({idx, idx + 1, "short"}); }
	else if (sym[idx] == 't') { ast.push_back({idx, idx + 1, "unsigned short"}); }
	else if (sym[idx] == 'u') { ast.push_back({idx, idx + 1, "special: vendor type (unimplemented)"}); return -1; }
	else if (sym[idx] == 'v') { ast.push_back({idx, idx + 1, "void"}); }
	else if (sym[idx] == 'w') { ast.push_back({idx, idx + 1, "wchar_t"}); }
	else if (sym[idx] == 'x') { ast.push_back({idx, idx + 1, "long long"}); }
	else if (sym[idx] == 'y') { ast.push_back({idx, idx + 1, "unsigned long long"}); }
	else if (sym[idx] == 'z') { ast.push_back({idx, idx + 1, "..."}); }
	else {
		return -1;
	}
	return ++idx;
}

static ssize_t dparse_paramlist(std::list<seg> &ast, const char *sym, size_t idx,
    char endchar)
{
	ast.push_back({idx, idx, "parameter type list"});
	auto &plist = *ast.rbegin();
	while (sym[idx] != endchar && sym[idx] != '\0') {
		int ret = dparse_type(plist.sub, sym, idx);
		if (ret < 0)
			return ret;
		idx = ret;
	}
	plist.end = idx;
	return idx;
}

static ssize_t dparse_template1(std::list<seg> &ast, const char *sym, size_t idx)
{
	ast.push_back({idx, idx, "template"});
	if (sym[idx] != 'I')
		return -1;
	auto &tl = *ast.rbegin();
	tl.sub.push_back({idx, idx + 1, "template parameter list start marker"});
	++idx;
	if (sym[idx] == '\0')
		return -1;
	ssize_t ret = dparse_paramlist(tl.sub, sym, idx, 'E');
	if (ret < 0)
		return ret;
	idx = ret;
	if (sym[idx] != 'E')
		return -1;
	tl.sub.push_back({idx, idx + 1, "template parameter list end marker"});
	++idx;

	tl.sub.push_back({idx, idx, "return type"});
	auto &rt_type = *tl.sub.rbegin();
	ret = dparse_type(rt_type.sub, sym, idx);
	if (ret < 0)
		return ret;
	idx = ret;
	tl.end = rt_type.end = idx;

	/* template means it's a function and MUST take params */
	if (sym[idx] == '\0')
		return -1;
	return dparse_paramlist(ast, sym, idx);
}

static ssize_t dparse_z(std::list<seg> &ast, const char *sym, size_t idx)
{
	if (sym[idx] == 'N') {
		int ret = dparse_nspace(ast, sym, idx);
		if (ret < 0)
			return ret;
		idx = ret;
	} else if (isdigit(sym[idx])) {
		int ret = dparse_digitname(ast, sym, idx);
		if (ret < 0)
			return ret;
		idx = ret;
	} else {
		return -1;
	}
	if (sym[idx] == 'I') {
		int ret = dparse_template1(ast, sym, idx);
		if (ret < 0)
			return ret;
		idx = ret;
	} else if (sym[idx] != '\0') {
		int ret = dparse_paramlist(ast, sym, idx);
		if (ret < 0)
			return ret;
		idx = ret;
	}
	return idx;
}

static ssize_t dparse_start(std::list<seg> &ast, const char *sym, size_t idx)
{
	if (sym[idx] == '_' && sym[idx+1] == 'Z') {
		ast.push_back({idx, idx + 2, "normal name marker"});
		int ret = dparse_z(ast, sym, idx + 2);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static void dprint(const std::list<seg> &list, const char *sym, size_t depth = 0)
{
	for (const auto &seg : list) {
		printf("%.*s%s%.*s%s%s: %-*s%s\n",
			static_cast<int>(seg.start), sym,
			c_hi, static_cast<int>(seg.end - seg.start), sym + seg.start, c_normal,
			sym + seg.end,
			static_cast<int>(depth * 2), "", seg.desc.c_str());
		if (seg.sub.size() > 0)
			dprint(seg.sub, sym, depth + 1);
	}
}

static void demangle(const char *sym)
{
	printf("==== %s ====\n", sym);
	std::list<seg> ast;
	int ret = dparse_start(ast, sym, 0);
	if (ret < 0)
		printf("Demangle failed along the way\n");
	dprint(ast, sym);
}
int main(int argc, const char **argv)
{
	if (argc == 1) {
		hxmc_t *line = NULL;
		while (HX_getl(&line, stdin) != NULL) {
			HX_chomp(line);
			demangle(line);
		}
	} else {
		while (*++argv != NULL)
			demangle(*argv);
	}
	return EXIT_SUCCESS;
}
