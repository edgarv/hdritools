function [ldr, a, Lwhite, Lwp, nZones] = reinhard02(hdr, varargin)
%REINHARD02  Applies the TMO from Reinhard et al. ACM SIGGRAPH 2002
% [LDR,A,LWHITE,LAV,NZONES] = REINHARD02(HDR, ...) applies
% the tone mapping operator described in "Photographic Tone 
% Reproduction for Digital Images" by Erik Reinhard et al., ACM SIGGRAPH 
% 2002, to the high dynamic image HDR, which is assumed to be in linear 
% radiance space. It produces a low dynamic range image LDR suitable for 
% display.
%
% In the full form it will also return the image key A, the white point
% LWHITE, average luminance LWP and the number of zones NZONES actually
% used in the image. Note that LDR is by default in linear space, thus it
% needs to be gamma-corrected before display.
%
% Parameters
% ----------
% The function accepts a numer of named parameters in the form of list
% 'param1', val1, 'param2', val2, ... These parameters may appear in any
% order after the HDR argument. The supported options are:
%     'key'       - subjective brighness. By default an appropriate 
%                   value is calculated automatically.
%     'white'     - minimum value to map to white. By default an
%                   appropriate is calculated automatically.
%     'Lav'       - log-average luminance of the image. By default this
%                   value is computed directly from the image.
%     'gamma'     - <value|'sRGB'> gamma corrects the LDR image by raising
%                   each pixel to the power of 1/value. If the parameter's
%                   value is 'sRGB' it will apply the sRGB transform.
%                   (Default 1.0, i.e. no transform.)
%     'scale'     - <'auto'|true|false> If 'auto', it will chose either
%                   the simple operator or the dodge and burn one
%                   automatically depending on the number of zones. If the
%                   parameter is the logical value 'true' it will use the
%                   dodge and burn operator. If set to the logical 'false'
%                   it will use the simple operator. (Default false.)
% 
% The following parameters relate only to the dodge and burn operator:
%     'low'       - smallest gaussian in pixels (default 1).
%     'high'      - largest gaussian in pixels (default 43).
%     'num'       - number of scales to use (default 8).
%     'phi'       - sharpening parameter (default 8.0)
%     'threshold' - threshold for choosing the scale (default 0.05).
%
% Class Support
% -------------
% The high dynamic range image HDR must be a m-by-n-by-3 single or
% double array.  The output image LDR is an m-by-n-by-3 real image.
%
% Example
% -------
% Load a high dynamic range image, convert it to a low dynamic range
% image and applies gamma=2.2 before display.
%
%     hdr = hdrread('office.hdr');
%     ldr = reinhard02(hdr);
%     imshow(ldr.^(1/2.2));
%
% See also hdrread, tonemap

% ============================================================================
% HDRITools - High Dynamic Range Image Tools
% Copyright 2008-2013 Program of Computer Graphics, Cornell University
%
% Distributed under the OSI-approved MIT License (the "License");
% see accompanying file LICENSE for details.
%
% This software is distributed WITHOUT ANY WARRANTY; without even the
% implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
% See the License for more information.
% ----------------------------------------------------------------------------
% Primary author:
%     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
% ============================================================================


%% Output argument checking
error(nargchk(0, 5, nargout));


%% Input argument parser construction

% Create and setup an instance of the inputParser class
p = inputParser;
p.FunctionName = 'reinhard02';

% Setup the required hdr parameter
p.addRequired('hdr', @(x)(ndims(x) <= 3 && ...
    (size(x,3) == 1 || size(x,3) == 3) && isreal(x)));

% The "key" and "whitePoint" are user params for which default values may
% be obtained later
p.addParamValue('key', nan, @(x)(isscalar(x) && isreal(x) && x > 0));
p.addParamValue('white', nan, @(x)(isscalar(x) && isreal(x) && x > 0));

% The log-average "Lav" may be provided to guarantee, along witht the
% key and the whitePoint, that the tone mapping curves between different
% images are the same
p.addParamValue('Lav', nan, @(x)(isscalar(x) && isreal(x) && x > 0));

% Flat to know the behavior of the local version of the operator. By
% default uses the simple operator.
p.addParamValue('scale', false, @(x)((islogical(x) && ...
    isscalar(x)) || (ischar(x) && strcmpi(x, 'auto'))));

% The rest are parameters for the dodge and burn operator, normally they
% shouldn't be modified

validint = @(x)(isscalar(x) && isreal(x) && (mod(x,1) == 0));

% Smallest gaussian
p.addParamValue('low', 1, @(x)(x > 0 && validint(x)));
% Largest gaussian
p.addParamValue('high', 43, @(x)(x > 0 && validint(x)));
% Number of scales to use
p.addParamValue('num', 8, @(x)(x >= 2 && validint(x)));
% Sharpening parameter
p.addParamValue('phi', 8., @(x)(isscalar(x) && isnumeric(x) && isreal(x)));
% Threshold for choosing a scale
p.addParamValue('threshold', 0.05, @(x)(isscalar(x) && isreal(x) && x>0));

% Optional gamma to apply to the output to produce non-linear results
p.addParamValue('gamma', 1.0, @(x)((ischar(x) && strcmpi(x,'srgb')) || ...
    (isscalar(x) && isnumeric(x) && isreal(x) && x>0)));


%% Actually parse the arguments
p.parse(hdr, varargin{:});

%% Luminance calculation

% Is there a more elegant way to do this?
if size(hdr,3) == 3
    Lw = 0.27*hdr(:,:,1) + 0.67*hdr(:,:,2) + 0.06*hdr(:,:,3);
else
    assert(size(hdr,3) == 1, 'PCG:illegalState', 'Not a monochrome image');
    Lw = hdr;
end


% Automatic parameters:
% This follows "Parameter estimation for photographic tone reproduction" by
% Erik Reinhard, Journal of Graphics Tools Volume 7, Issue 1 (Nov 2002)
% http://www.cs.bris.ac.uk/~reinhard/papers/jgt_reinhard.pdf

%% Log luminance of finite, positive values

% Take the non-zero, finite elements as a single column and sort them
Lwv = nonzeros(Lw(Lw > 0 & isfinite(Lw)));
if isempty(Lwv)
    error('PCG:invalidImage', 'Cannot estimate parameters for image');
end

% Natural logarithm of all valid luminance values
log_Lwv = log(Lwv);
log2_range = 1/log(2)*range(log_Lwv);

%% Average log luminance
% Equation 1 of the paper, without \delta since there are no zeros
if isnan(p.Results.Lav)
    N = length(Lwv);
    Lwp = exp((1/N)*sum(log_Lwv));
else
    Lwp = p.Results.Lav;
end

%% Key estimation
if isnan(p.Results.key)
    % Equation 4 of the JGT paper
    % This uses the reduced range of luminances
    [L1_log2, L99_log2] = histogram(log_Lwv);
    
    % If also estimating the average log luminance, remove extreme values
    % as in the paper
    if isnan(p.Results.Lav)
        log_lum_cutoff = 2^L99_log2;
        log_Lwv_below = log_Lwv(log_Lwv <= log_lum_cutoff);
        N = length(log_Lwv_below);
        Lwp = exp((1/N)*sum(log_Lwv_below));
    end
    a = 0.18 * 4 ^ ((2*log2(Lwp) - L1_log2 - L99_log2) / ...
        (L99_log2 - L1_log2));
else
    a = p.Results.key;
end

%% White point estimation
if isnan(p.Results.white)
    % Equation 5 of the JGT paper
    % This uses the full range of luminance
    Lwhite = 1.5 * 2 ^ (log2_range-5);
else
    Lwhite = p.Results.white;
end

%% Tone mapping operation

% Just for reference, the number of zones
nZones = log2_range;

useScales = nZones > 11;
% Override the decision
if islogical(p.Results.scale)
    useScales = p.Results.scale;
end

if ~useScales
%% Global tone mapping    

    % This expressions combine equations 2 and 4 of the paper with the 
    % scaling: The canonical approach is:
    %   a. Transform sRGB to xyY
    %   b. Apply the TMO to Y
    %   c. Transform x,y,TMO(Y) back to sRGB
    %
    % However, having only Y and assuming that TMO(Y) == k*Y, then the
    % result of all the transformation is just k*[r,g,b]
    % Thus:
    %         (key/avgLogLum) * (1 + (key/avgLogLum)/pow(Lwhite,2) * Y)
    %    k == ---------------------------------------------------------
    %                        1 + (key/avgLogLum)*Y
    % Where:
    %    k == (P * (R + Q*(P*Y)) / (R + P*Y)
    %    P == key / avgLogLum
    %    Q == 1 / pow(Lwhite,2)
    %    R == 1
    
    P  = a / Lwp;
    Q  = 1.0/(Lwhite*Lwhite);
    Lp = P .* Lw;
    Ls = (P * (1 + Q.*Lp)) ./ (1 + Lp);
    
else
%% Local dodge & burn

    % Use values from the input arguments parser
    num_scales    = p.Results.num;
    lowest_scale  = p.Results.low;
    highest_scale = p.Results.high;
    phi           = p.Results.phi;
    threshold     = p.Results.threshold;
    
    scale_spacing = (highest_scale/lowest_scale)^(1/num_scales);
    s_n = @(n)(lowest_scale * scale_spacing.^n);

    % Map to midtone
    L = (a / Lwp) * Lw;
    
    V_tensor = V(L, num_scales, lowest_scale, highest_scale);
    V1 = @(n)(V_tensor(:,:,n));
    V2 = @(n)(V1(n+1));
    
    % This is equation 7 of the paper
    activity = @(n)((V1(n)-V2(n)) ./ ...
        (2^phi*a/(s_n(n)^2) + V1(n)));
    
    % Find the V1 of the prefered scale (Eq. 8)
    
    % This method is slow, but a naive nested loop seems even slower!
    selected = [];
    V1_pref = V1(num_scales);
    for n=1:num_scales-1
        candidates = setdiff(find(abs(activity(n))>threshold), selected);
        if ~isempty(candidates)
            V1_curr = V1(n);
            V1_pref(candidates) = V1_curr(candidates);
            selected = union(selected, candidates);
        end
    end
    
    % Final tone map using equation 9
    Ld = L ./ (1 + V1_pref);
    
    % Hacky-scaling
    Ls = Ld./Lw;
end

%% LDR Assembly
ldr = zeros(size(hdr));
for i=1:size(hdr,3)
    ldr(:,:,i) = Ls .* hdr(:,:,i);
end


%% Gamma and class modifications
if p.Results.gamma ~= 1.0
    if isnumeric(p.Results.gamma)
        ldr = ldr .^ (1/p.Results.gamma);
    else
        ldr = srgb(ldr);
    end
end

if isa(hdr, 'single')
   ldr = single(ldr); 
end
  
end


%% Histogram-based extraction of the reduced range for the key estimation
function [log2_L1, log2_L99] = histogram(log_Lwv)
% Helper function which returns the log2 luminance corresponding to the
% percentiles 1 and 99 of the log luminance
% The argument is the array of the [natural] log of the luminances, already
% without any NaN or infinity values.
% This function mimics the refernce implementation.

dynrange = ceil(1e-5 + range(log_Lwv));
resolution = 100;
num_bins = dynrange*resolution;
h = hist(log_Lwv, num_bins);
threshold = 0.01 * length(log_Lwv);
bins_idx = sum(cumsum([h ; h(end:-1:1)], 2)'  < threshold);
bins_idx = bins_idx .* [1 -1] + [0 num_bins-1];
L1_L99 = (min(log_Lwv) + (range(log_Lwv) / num_bins * bins_idx)) / log(2);

log2_L1  = L1_L99(1);
log2_L99 = L1_L99(2);

end


%% Dodge-and-burn center surround computation
function V_tensor = V(L, num_scales, lowest_scale, highest_scale)
% L is the midtone image, this is the luminance image Lw scaled by
% key/[log-average luminance]

scale_spacing = (highest_scale/lowest_scale)^(1/num_scales);
s_n = @(n)(lowest_scale * scale_spacing.^n);

%% Build the gaussians
alpha = 1 / (2*sqrt(2));
a = @(n)(1 ./ (alpha * s_n(n)));

% Gaussian profile in just 1 dimension (sans the 1/2 factor)
gaussian_profile_1d = @(v,n)(erf(a(n)*(v+0.5))-erf(a(n)*(v-0.5)));

x = (0:size(L,2)-1);
x(x>=length(x)/2) = x(x>=length(x)/2) - length(x);
y = (0:size(L,1)-1)';
y(y>=length(y)/2) = y(y>=length(y)/2) - length(y);

% Full filter using outer product (y-column vector, x-row vector)
gaussian_profile = ...
    @(n)(0.25*gaussian_profile_1d(y,n)*gaussian_profile_1d(x,n));


%% Convolve with the image
L_fft = fft2(L);
V_tensor = zeros([size(L) num_scales]);
for n=0:num_scales-1
    V_tensor(:,:,n+1) = ifft2(L_fft .* fft2(gaussian_profile(n)));
end

end


%% sRGB conversion for a whole image
function ldr=srgb(img)

mask = img <= 0.0031308;
a    = 0.055;
ldr  = 12.92*(img.*mask) + (1+a)*((img.*(1-mask)).^(1/2.4)) - a;

end
