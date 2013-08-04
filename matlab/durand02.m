function ldr = durand02(hdr, contrast)
%DURAND02   Applies the TMO from Durand and Dorsey, ACM SIGGRAPH 2002.
%  DURAND02 applies the tone mapping operator described in "Fast 
%  Bilateral Filtering for the Display of High-Dynamic-Range Images" by
%  Fredo Durand and Julie Dorsey, ACM SIGGRAPH 2002 (TOG Vol 21 No. 3)
%  DOI: 10.1145/566654.566574. Quoting from the paper's abstract:
%
%      It is based on a two-scale decomposition of the image into a 
%      base layer, encoding large-scale variations, and a detail 
%      layer. Only the base layer has its contrast reduced, thereby
%      preserving detail. The base layer is obtained using an
%      edge-preserving filter called the bilateral filter. This is a
%      non-linear filter, where the weight of each pixel is computed
%      using a Gaussian in the spatial domain multiplied by an
%      influence function in the intensity domain that decreases the
%      weight of pixels with large intensity differences.
%
%  RGB = DURAND02(HDR) performs tone mapping on the hight dynamic range
%  image HDR using the default contrast target 50.0.
%
%  RGB = DURAND02(HDR, constrast) performs tone mapping on the hight 
%  dynamic range image HDR using the specified contrast target. Meaninful
%  values of contrast are reals in the range [5,200].
%
%  Class Support
%  -------------
%  The high dynamic range image HDR must be a m-by-n-by-3 single or
%  double array.  The output image LDR is an m-by-n-by-3 double image.
% 
%  Example
%  -------
%  Load a high dynamic range image, convert it to a low dynamic range
%  image and applies gamma=2.2 before display.
% 
%      hdr = hdrread('office.hdr');
%      ldr = durand02(hdr);
%      imshow(ldr.^(1/2.2));
% 
%  See also hdrread, tonemap

% =========================================================================
% HDRITools - High Dynamic Range Image Tools
% Copyright 2008-2013 Program of Computer Graphics, Cornell University
%
% Distributed under the OSI-approved MIT License (the "License");
% see accompanying file LICENSE for details.
%
% This software is distributed WITHOUT ANY WARRANTY; without even the
% implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
% See the License for more information.
% -------------------------------------------------------------------------
% Primary author:
%     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
% =========================================================================

% =========================================================================
% Approximated bilateral filter as defined in:
% Sylvain Paris and Fredo Durand. "A Fast Approximation of the Bilateral 
% Filter using a Signal Processing Approach". European Conference on 
% Computer Vision (ECCV'06).
% http://people.csail.mit.edu/sparis/bf/#eccv06 (Accessed July 2013)
%
% This script uses portions of the MIT-licensed reference code 
% by Jiawen Chen, Sylvain Paris and Fredo Durand (Accessed July 2013):
% http://people.csail.mit.edu/sparis/#code
% http://people.csail.mit.edu/jiawen/#code
% -------------------------------------------------------------------------
% Copyright (c) 2007 Jiawen Chen, Sylvain Paris, and Fredo Durand
%
% Permission is hereby granted, free of charge, to any person obtaining a copy
% of this software and associated documentation files (the "Software"), to deal
% in the Software without restriction, including without limitation the rights
% to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
% copies of the Software, and to permit persons to whom the Software is
% furnished to do so, subject to the following conditions:
% 
% The above copyright notice and this permission notice shall be included in
% all copies or substantial portions of the Software.
% 
% THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
% IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
% FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
% AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
% LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
% OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
% THE SOFTWARE.
% =========================================================================

%% Parameter setup from arguments

% Basic argument checking
error(nargchk(0, 1, nargout));
error(nargchk(1, 2, nargin));

% Input arguments. The second optional argument is contrast
if ~(ndims(hdr) == 3 && size(hdr, 3) == 3 && isreal(hdr) && isfloat(hdr))
    error('PCG:invalidImage', 'hdr is not an RGB floating point image');
end

if nargin == 2,
    if ~(isscalar(contrast) && isreal(contrast) && ...
         isnumeric(contrast) && contrast > 0.0)
        error('PCG:invalidParameter', ...
            'contrast is not a scalar number greater than zero');
    end
    contrast = double(contrast);
    if contrast < 5.0 || contrast > 200.0
        warning('PCG:invalidParameter', ...
            'contrast should be between 5.0 and 200.0: %g', contrast);
    end
else
    % Default, good enough value
    contrast = 50.0;
end

height = size(hdr, 1);
width  = size(hdr, 2);

% TODO Allow a custom min/max intensity value


%% Compute the log-intensity channel
intensity = (20./61)*double(hdr(:,:,1)) + ...
            (40./61)*double(hdr(:,:,2)) + ...
             (1./61)*double(hdr(:,:,3));
log_intensity = log10(intensity);


%% Choose sane default values
space_sigma = 0.02 * min(width, height);
range_sigma = 0.4;

% Handle zeros, nans and inf
log_intensity_mask = ~isnan(log_intensity) & ~isinf(log_intensity);
LogI_valid = log_intensity(log_intensity_mask);
if isempty(LogI_valid)
    error('PCG:invalidImage', 'The image does not contain normal values');
elseif range(LogI_valid) < 1e-10
    error('PCG:invalidImage', 'The image hast the same overall luminance');
end
% Delete the extreme 1.5% at both ends to remove weird values
[~, bins] = hist(LogI_valid, 100);
input_min = bins(2);
input_max = bins(99);


%% Filter the log-intensity channel
filtered_log_intensity = linear_BF(log_intensity, ...
    space_sigma, range_sigma, input_min, input_max);
FLogI_valid = filtered_log_intensity(~isnan(filtered_log_intensity) & ...
                                     ~isinf(filtered_log_intensity));

% TODO Implement the edge correction


%% Compute the new intensity channel
detail = log_intensity - filtered_log_intensity;
detail(~log_intensity_mask) = log_intensity(~log_intensity_mask);
min_value = min(FLogI_valid);
max_value = max(FLogI_valid);

% The multiplication by compressionFactor can be replaced by any
% contrast-reduction curve (e.g. histogram adjustment, Reinhard et al.'s
% saturation function, etc.) 
compressionFactor  = log10(contrast) / (max_value - min_value);
% log_absolute_scale essentially normalizes the image, by making sure that
% the biggest value in the base after compression is 1
log_absolute_scale = max_value * compressionFactor;
new_intensity = 10.0 .^ (compressionFactor * filtered_log_intensity ...
    + detail - log_absolute_scale);

%% Recompose the color image
ldr = zeros(height, width, 3);
% Avoid NaNs
mask = isnan(intensity) | isinf(intensity) | intensity == 0;
original_values = intensity(mask);
intensity(mask) = -1;
ratio = new_intensity ./ intensity;
ratio(mask) = original_values;
for k=1:3
    ldr(:,:,k) = ratio .* hdr(:,:,k);
end

end



function filtered = linear_BF(A, space_sigma, range_sigma, ...
    input_min, input_max)
% filtered = linear_BF(A, space_sigma, range_sigma, in_min, in_max)
% Computes the bilateral filter of A using a truncated 7^3 kernel.
% This function uses the same sampling factor as the sigmas, thus both
% the derived sigma_range and sigma_space are one. Therefore a a truncated
% kernel covering 3*sigma is 7 pixels wide.

%% Initialization

% Recall that
%   space_sampling = space_sigma;
%   range_sampling = range_sigma;
inv_space_sampling = 1.0 / space_sigma;
inv_range_sampling = 1.0 / range_sigma;

height = size(A, 1);
width  = size(A, 2);
if input_min >= input_max
    error('PCG:invalidParameter', 'Invalid input min/max values');
end
input_delta = input_max - input_min;

% With a kernel width of 7 it is enough to add 3 pixels of zero padding
% at each end
padding = 3;

small_width  = fix((width-1)  * inv_space_sampling)  + 1 + 2*padding;
small_height = fix((height-1) * inv_space_sampling)  + 1 + 2*padding;
small_depth  = fix(input_delta * inv_range_sampling) + 1 + 2*padding;

W  = zeros(small_height, small_width, small_depth);
IW = zeros(small_height, small_width, small_depth);

%% Downsampling

% compute downsampled indices
[jj, ii] = meshgrid(0 : width-1, 0 : height-1);

% meshgrid does x, then y so output arguments need to be reversed
% so when iterating over ii( k ), jj( k )
% get: ( 0, 0 ), ( 1, 0 ), ( 2, 0 ), ... (down columns first)
di = round(ii * inv_space_sampling) + padding + 1;
dj = round(jj * inv_space_sampling) + padding + 1;
dz = round((A - input_min) * inv_range_sampling) + padding + 1;

% perform scatter/gather (there's probably a faster way than this)
for k = 1 : numel( dz )
    dataZ = A(k); % traverses the image column wise, same as di(k)
    if ~isnan(dataZ) && dz(k) > padding && dz(k) < (small_depth-padding)
        dik = di(k);
        djk = dj(k);
        dzk = dz(k);

        IW(dik, djk, dzk) = IW(dik, djk, dzk) + dataZ;
        W( dik, djk, dzk) = W( dik, djk, dzk) + 1;
    end
end


%% Convolution

% 1D gaussian, sigma=1, 

% Build the 3D kernel via separability, combining the 1D discrete gaussian
% with t = sigma^2 = 1, truncated to 3*sigma. See:
% http://en.wikipedia.org/wiki/Scale_space_implementation#The_discrete_Gaussian_kernel

% Precomputed discrete gaussian T(n,t) = exp(-t)*besseli(n,t)
%   exp(-1)*besseli(-3:3,1)
g1D = [0.0081553077728143
       0.0499387768942236
        0.207910415349708
         0.46575960759364
        0.207910415349708
       0.0499387768942236
       0.0081553077728143];
g2D = g1D * g1D';
g3D = zeros(7, 7, 7);
for k=1:7
    g3D(:,:,k) = g1D(k) * g2D;
end

% Convolve: use the explicit 3D kernel to get succinct Matlab code. A fast
% implementation would only use the 1D filter to convolve the grid in
% three successive passes.
blurredIW = convn(IW, g3D, 'same');
blurredW  = convn(W,  g3D, 'same');


%% Normalize weights

% Normalization: because both IW and W are convolved with the same filter
% it does not matter if such filter is not normalized per se as IW/W will
% eliminate the normalization constant of the filter.
blurredW(blurredW == 0) = -1; % avoid divide by 0, won't read there anyway
blurredI = blurredIW ./ blurredW;
blurredI(blurredW < 0) = 0; % put 0s where it's undefined

%% Upsampling indices

% Upsample: meshgrid does x, then y so output arguments need to be reversed
[jj, ii] = meshgrid(0 : width-1, 0 : height-1);
% no rounding as we want fractional coordinates for interpolation
di = (ii * inv_space_sampling) + padding + 1;
dj = (jj * inv_space_sampling) + padding + 1;
Z_clamped = max(0, min(input_delta, (A - input_min)));
dz = (Z_clamped * inv_range_sampling) + padding + 1;

%% Interpolation and result

%Trilinear interpolation: interpn takes rows, then cols, etc
% i.e. size(v,1), then size(v,2), ...
filtered = interpn(blurredI, di, dj, dz);

% Restore the original invalid values (e.g. zeros)
valid_mask = ~isnan(A) & ~isinf(A);
filtered(~valid_mask) = A(~valid_mask);

end
