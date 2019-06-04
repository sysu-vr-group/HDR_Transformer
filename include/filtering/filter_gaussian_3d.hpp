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

#ifndef PIC_FILTERING_FILTER_GAUSSIAN_3D_HPP
#define PIC_FILTERING_FILTER_GAUSSIAN_3D_HPP

#include "../filtering/filter_npasses.hpp"
#include "../filtering/filter_gaussian_1d.hpp"

namespace pic {

/**
 * @brief The FilterGaussian3D class
 */
class FilterGaussian3D: public FilterNPasses
{
    FilterGaussian1D *gaussianFilter;

public:
    /**
     * @brief FilterGaussian3D
     */
    FilterGaussian3D()
    {
        gaussianFilter = NULL;
    }

    /**
     * @brief FilterGaussian3D
     * @param sigma
     */
    FilterGaussian3D(float sigma)
    {
        //Gaussian filter
        gaussianFilter = new FilterGaussian1D(sigma);

        insertFilter((Filter *)gaussianFilter);
        insertFilter((Filter *)gaussianFilter);
        insertFilter((Filter *)gaussianFilter);
    }

    ~FilterGaussian3D()
    {
        if(gaussianFilter != NULL) {
            delete gaussianFilter;
        }
    }

    /**
     * @brief execute
     * @param imgIn
     * @param imgOut
     * @param sigma
     * @return
     */
    static Image *execute(Image *imgIn, Image *imgOut, float sigma)
    {
        FilterGaussian3D filter(sigma);
        Image *ret = filter.Process(Single(imgIn), imgOut);
        return ret;
    }

    /**
     * @brief execute
     * @param nameIn
     * @param nameOut
     * @param sigma
     * @return
     */
    static Image *execute(std::string nameIn, std::string nameOut, float sigma)
    {
        Image imgIn(nameIn);
        Image *imgOut = execute(&imgIn, NULL, sigma);
        imgOut->Write(nameOut);
        return imgOut;
    }
};

} // end namespace pic

#endif /* PIC_FILTERING_FILTER_GAUSSIAN_3D_HPP */

