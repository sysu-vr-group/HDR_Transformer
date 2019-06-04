/*

PICCANTE
The hottest HDR imaging library!
http://vcg.isti.cnr.it/piccante

Copyright (C) 2014
Visual Computing Laboratory - ISTI CNR
http://vcg.isti.cnr.it
First author: Francesco Banterle

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef PIC_FILTERING_FILTER_LOG_2D_HPP
#define PIC_FILTERING_FILTER_LOG_2D_HPP

#include "../filtering/filter_diff_gauss_2d.hpp"

namespace pic {

/**
 * @brief The FilterLoG2D class
 */
class FilterLoG2D: public FilterDiffGauss
{

public:
    float sigma;

    /**
     * @brief FilterLoG2D
     * @param sigma
     */
    FilterLoG2D(float sigma) : FilterDiffGauss(sigma * sqrtf(2.0f), sigma / sqrtf(2.0f))
    {
        this->sigma = sigma;
    }

    /**
     * @brief execute
     * @param imgIn
     * @param imgOut
     * @param sigma_1
     * @param sigma_2
     * @return
     */
    static Image *execute(Image *imgIn, Image *imgOut, float sigma)
    {
        FilterLoG2D filter(sigma);
        return filter.Process(Single(imgIn), imgOut);
    }
};

} // end namespace pic

#endif /* PIC_FILTERING_FILTER_LOG_2D_HPP */

