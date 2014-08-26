#include "gtest_utils.h"
#include "../Renderer/Backend/Backend.h"

static const auto NUM_BAD_PIXEL_THRESHOLD = 50;
static const double INDIVIDUAL_PIXEL_ERR_THRESHOLD = 1e-3;
static const double TOTAL_ERR_THRESHOLD = 0.0009;

// misc helpers
// trying to ensure the highest precision possible
inline double _my_max(double lhs, double rhs) { return (lhs > rhs) ? lhs: rhs; }
inline double _my_abs(double t) { return (t > 0) ? t : -t ; }
// ad-hoc difference metric
inline double pixel_diff(const RGBPixel& lhs, const RGBPixel& rhs) {
    static const double k = 1.0/255.0;
    double tmp[] = {
        k*_my_abs(lhs.x - rhs.x),
        k*_my_abs(lhs.y - rhs.y),
        k*_my_abs(lhs.z - rhs.z),
    };
    return _my_max(tmp[0], _my_max(tmp[1], tmp[2]));
}

bool image_match(RGBPixel* lhs, RGBPixel* rhs, uint16_t w, uint16_t h) {
    const auto n = w*h;
    auto num_bad_pixels = 0L;
    double total_err = 0.0;
    bool do_they_match = true;

    if (!lhs || !rhs)
        return false;

    for (auto i = 0;i < n;++i) {
        auto err = pixel_diff(lhs[i], rhs[i]);
        total_err += err;
        if (err >= INDIVIDUAL_PIXEL_ERR_THRESHOLD)
            ++num_bad_pixels;

        if ((num_bad_pixels >= NUM_BAD_PIXEL_THRESHOLD) || (total_err >= TOTAL_ERR_THRESHOLD*n))
            do_they_match = false;
    }
    if (do_they_match)
        LOG(LOG_INFORMATION, "%d bad pixels and %lf percent error: within threshold ", num_bad_pixels, total_err/n);
    else
        LOG(LOG_WARNING, "Threshold broken: %d bad pixels with %lf%% error", num_bad_pixels, total_err/n);
    return do_they_match;
}

bool image_match(const char* lhs, const char* rhs, uint16_t w, uint16_t h) {
    auto l = Backend::read_screenshot(lhs);
    auto r = Backend::read_screenshot(rhs);
    bool ret = image_match(l, r, w, h);
    delete[] l;
    delete[] r;
    return ret;
}