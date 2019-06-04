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

#ifndef PIC_FILTERING_FILTER_GL_MIN_HPP
#define PIC_FILTERING_FILTER_GL_MIN_HPP

#include "../../gl/filtering/filter_npasses.hpp"
#include "../../gl/filtering/filter_non_linear_1d.hpp"

namespace pic {

/**
 * @brief The FilterGLMin class
 */
class FilterGLMin: public FilterGLNPasses
{
protected:

protected:
    FilterGLNonLinear1D *filter;
    int                 kernelSize;

public:

    /**
     * @brief FilterGLMin
     * @param kernelSize
     */
    FilterGLMin(int kernelSize) : FilterGLNPasses()
    {
        filter = new FilterGLNonLinear1D(kernelSize, "min", GL_TEXTURE_2D);

        insertFilter(filter);
        insertFilter(filter);
    }

    ~FilterGLMin()
    {
        if(filter != NULL) {
            delete filter;
            filter = NULL;
        }
    }


    /**
     * @brief execute
     * @param imgIn
     * @param imgOut
     * @param kernelSize
     * @return
     */
    static ImageGL *execute(ImageGL *imgIn, ImageGL *imgOut, int kernelSize)
    {
        FilterGLMin filter(kernelSize);
        return filter.Process(SingleGL(imgIn), imgOut);
    }
};

} // end namespace pic

#endif /* PIC_FILTERING_FILTER_GL_MIN_HPP */

