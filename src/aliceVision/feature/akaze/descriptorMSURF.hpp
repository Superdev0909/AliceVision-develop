// This file is part of the AliceVision project.
// Copyright (c) 2016 AliceVision contributors.
// Copyright (c) 2012 openMVG contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <aliceVision/feature/Descriptor.hpp>
#include <aliceVision/feature/PointFeature.hpp>
#include <aliceVision/numeric/MathTrait.hpp>

namespace aliceVision {
namespace feature {

/**
 * @brief This function computes the value of a 2D Gaussian function
 * @param x X Position
 * @param y Y Position
 * @param sigma Standard Deviation
 */
template<typename Real>
inline Real gaussian(const Real x, const Real y, const Real sigma)
{
    return MathTrait<Real>::exp(-((x * x) + (y * y)) / (static_cast<Real>(2) * sigma * sigma));
}

/**
 * @brief This method computes the descriptor of the provided keypoint given the main orientation of the keypoint
 * @param Lx Input X-derivative
 * @param Ly Input Y-derivative
 * @param id_octave Id of given octave
 * @param ipt Input interest point
 * @param desc Ouput descriptor
 * @note Rectangular grid of 24 s x 24 s. Descriptor Length 64. The descriptor is inspired
 * from Agrawal et al., CenSurE: Center Surround Extremas for Realtime Feature Detection and Matching,
 * ECCV 2008
 */
template<typename ImageT, typename Real>
inline void ComputeMSURFDescriptor(const ImageT& Lx, const ImageT& Ly, const int id_octave, const PointFeature& ipt, Descriptor<Real, 64>& desc)
{
    Real dx = 0, dy = 0, mdx = 0, mdy = 0, gauss_s1 = 0, gauss_s2 = 0;
    Real rx = 0, ry = 0, rrx = 0, rry = 0, ys = 0, xs = 0;
    Real sample_x = 0, sample_y = 0;
    int kx = 0, ky = 0, i = 0, j = 0, dcount = 0;

    // Subregion centers for the 4x4 gaussian weighting
    Real cx = -static_cast<Real>(0.5), cy = static_cast<Real>(0.5);

    // Set the descriptor size and the sample and pattern sizes
    const int sample_step = 5;
    const int pattern_size = 12;

    // Get the information from the keypoint
    const Real ratio = static_cast<Real>(1 << id_octave);
    const int scale = MathTrait<float>::round(ipt.scale() / ratio);
    const Real angle = ipt.orientation();
    const Real yf = ipt.y() / ratio;
    const Real xf = ipt.x() / ratio;
    const Real co = MathTrait<Real>::cos(angle);
    const Real si = MathTrait<Real>::sin(angle);

    i = -8;

    const image::Sampler2d<image::SamplerLinear> sampler;

    // Calculate descriptor for this interest point
    // Area of size 24 s x 24 s
    while (i < pattern_size)
    {
        j = -8;
        i = i - 4;

        cx += 1.0;
        cy = -0.5;

        while (j < pattern_size)
        {
            dx = dy = mdx = mdy = 0.0;
            cy += 1.0;
            j = j - 4;

            ky = i + sample_step;
            kx = j + sample_step;

            xs = xf + (-kx * scale * si + ky * scale * co);
            ys = yf + (kx * scale * co + ky * scale * si);

            for (int k = i; k < i + 9; ++k)
            {
                for (int l = j; l < j + 9; ++l)
                {
                    // Get coords of sample point on the rotated axis
                    sample_y = yf + (l * scale * co + k * scale * si);
                    sample_x = xf + (-l * scale * si + k * scale * co);

                    // Get the gaussian weighted x and y responses
                    gauss_s1 = gaussian(xs - sample_x, ys - sample_y, static_cast<Real>(2.5) * static_cast<Real>(scale));

                    rx = sampler(Lx, sample_y, sample_x);
                    ry = sampler(Ly, sample_y, sample_x);

                    // Get the x and y derivatives on the rotated axis
                    rry = gauss_s1 * (rx * co + ry * si);
                    rrx = gauss_s1 * (-rx * si + ry * co);

                    // Sum the derivatives to the cumulative descriptor
                    dx += rrx;
                    dy += rry;
                    mdx += MathTrait<Real>::abs(rrx);
                    mdy += MathTrait<Real>::abs(rry);
                }
            }

            // Add the values to the descriptor vector
            gauss_s2 = gaussian(cx - static_cast<Real>(2.0), cy - static_cast<Real>(2.0), static_cast<Real>(1.5));
            desc[dcount++] = dx * gauss_s2;
            desc[dcount++] = dy * gauss_s2;
            desc[dcount++] = mdx * gauss_s2;
            desc[dcount++] = mdy * gauss_s2;

            j += 9;
        }

        i += 9;
    }

    // convert to unit vector (L2 norm)
    typedef Eigen::Matrix<Real, Eigen::Dynamic, 1> VecReal;
    Eigen::Map<VecReal> dataMap(&desc[0], 64);
    dataMap.normalize();
    // ALICEVISION_LOG_DEBUG(dataMap.transpose()));
}

template<typename ImageT>
inline void ComputeMSURFDescriptor(const ImageT& Lx,
                                   const ImageT& Ly,
                                   const int id_octave,
                                   const PointFeature& ipt,
                                   Descriptor<unsigned char, 64>& desc)
{
    Descriptor<float, 64> descFloat;
    ComputeMSURFDescriptor(Lx, Ly, id_octave, ipt, descFloat);
}

}  // namespace feature
}  // namespace aliceVision
