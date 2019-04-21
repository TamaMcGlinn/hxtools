#ifndef VFALIB_HPP
#define VFALIB_HPP 1

#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>
#include <endian.h>
#if (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) || \
    (defined(_BYTE_ORDER) && _BYTE_ORDER == _BIG_ENDIAN)
	/* We need to use constexpr functions, and htole16 unfortunately is not. */
#	define cpu_to_le16(x) __builtin_bswap16(x)
#	define cpu_to_le32(x) __builtin_bswap32(x)
#	define cpu_to_le64(x) __builtin_bswap64(x)
#	define cpu_to_be64(x) (x)
#	define le16_to_cpu(x) __builtin_bswap16(x)
#	define le32_to_cpu(x) __builtin_bswap32(x)
#	define le64_to_cpu(x) __builtin_bswap64(x)
#	define be64_to_cpu(x) (x)
#else
#	define cpu_to_le16(x) (x)
#	define cpu_to_le32(x) (x)
#	define cpu_to_le64(x) (x)
#	define cpu_to_be64(x) __builtin_bswap64(x)
#	define le16_to_cpu(x) (x)
#	define le32_to_cpu(x) (x)
#	define le64_to_cpu(x) (x)
#	define be64_to_cpu(x) __builtin_bswap64(x)
#endif


namespace vfalib {

struct vfpos {
	vfpos() = default;
	vfpos(const vfpos &) = default;
	vfpos(int a, int b) : x(a), y(b) {}
	int x = 0, y = 0;
};

struct vfsize {
	vfsize() = default;
	vfsize(const vfsize &) = default;
	vfsize(unsigned int a, unsigned int b) : x(a), y(b) {}
	unsigned int x = 0, y = 0;
};

struct unicode_map {
	std::map<unsigned int, std::set<char32_t>> m_i2u;
	std::map<char32_t, unsigned int> m_u2i;

	int load(const char *file);
	void add_i2u(unsigned int, char32_t);
	void add_u2i(char32_t cp, unsigned int i) { add_i2u(i, cp); }
	std::set<char32_t> to_unicode(unsigned int idx) const;
	ssize_t to_index(char32_t uc) const;
};

struct glyph {
	glyph() = default;
	glyph(const vfsize &size);
	static glyph create_from_rpad(const vfsize &size, const char *buf, size_t z);
	std::string as_pclt() const;
	std::string as_rowpad() const;
	glyph blit(const vfsize &sel, const vfpos &sof, const vfsize &cvs, const vfpos &pof) const;
	glyph upscale(const vfsize &factor) const;
	void lge();

	vfsize m_size;
	std::string m_data;
};

struct font {
	void init_256_blanks();
	ssize_t load_fnt(const char *file, unsigned int height_hint = -1);
	ssize_t load_hex(const char *file);
	ssize_t save_fnt(const char *file);
	ssize_t save_psf(const char *file);
	ssize_t save_clt(const char *dir);
	ssize_t save_clt_glyph(const char *dir, size_t n, char32_t cp);
	void blit(const vfsize &sel, const vfpos &sof, const vfsize &cvs, const vfpos &pof)
		{ for (auto &g : m_glyph) g = g.blit(sel, sof, cvs, pof); }
	void upscale(const vfsize &factor)
		{ for (auto &g : m_glyph) g = g.upscale(factor); }
	void lge();

	std::vector<glyph> m_glyph;
	std::shared_ptr<unicode_map> m_unicode_map;
};

/* P0052r5 (C++2020) */
template<typename F> class scope_success {
	private:
	F m_func;
	bool m_eod = false;

	public:
	explicit scope_success(F &&f) : m_func(std::move(f)), m_eod(true) {}
	scope_success(scope_success &&o) : m_func(std::move(o.m_func)), m_eod(o.m_eod) {}
	~scope_success() noexcept(noexcept(m_func()))
	{
#if __cplusplus >= 201700L
		if (m_eod && std::uncaught_exceptions() == 0)
			m_func();
#else
		if (m_eod && !std::uncaught_exception())
			m_func();
#endif
	}
	void operator=(scope_success &&) = delete;
};

template<typename F> scope_success<F> make_scope_success(F &&f)
{
	return scope_success<F>(std::move(f));
}

} /* namespace vfalib */

#endif /* VFALIB_HPP */