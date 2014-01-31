function M = isocontours(hdr, edge, frequency)
% ISOCONTOURS Generate isocontours on a linear HDR image (RGB-only)
% M = isocontours(hdr, edge, frequency)

%
% With code from "Basic Antialiasing in Shading Language" by Larry Gritz,
% SIGGRAPH 1998 Course 11: "Advanced RenderMan: Beyond the Companion".
% http://www.renderman.org/RMR/Publications/sig98.course11.pdf pages 62-80.
%

% ============================================================================
% HDRITools - High Dynamic Range Image Tools
% Copyright 2008-2014 Program of Computer Graphics, Cornell University
%
% Distributed under the OSI-approved MIT License (the "License");
% see accompanying file LICENSE for details.
%
% This software is distributed WITHOUT ANY WARRANTY; without even the
% implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
% See the License for more information.
% ----------------------------------------------------------------------------
% Primary author:
%     Edgar Velazquez-Armendariz <cornell#edu - eva5>
% ============================================================================

Y = lumImage(hdr);
Y = max(log(1e-12), real(log(Y)));

%Yfin = Y(isfinite(Y));
%[~,h] = hist(Yfin(:), 100);
%r = max(range(h(5:95)), 1e-12);
%factor = 1.0/r;
factor = 0.25;

%Y_mask = pulseTrain(frequency, edge, factor*Y);
filterWidth = max(abs(dFdx(factor*Y))+abs(dFdy(factor*Y)), 1e-6);
Y_mask = filteredPulseTrain(frequency, edge, factor*Y, filterWidth);
Y_mask = max(Y_mask, 0.02*ones(size(Y_mask)));

M = zeros(size(hdr));
M(:,:,1) = Y_mask .* hdr(:,:,1);
M(:,:,2) = Y_mask .* hdr(:,:,2);
M(:,:,3) = Y_mask .* hdr(:,:,3);

end

function M = lumImage(hdr)
% Combine the RGB image into a luminance one using lightcuts-light weights
% instead of the classic sRGB luminance ones, to avoid ignoring blues
M = (1.0/3)*hdr(:,:,1) + (1-2.0/3)*hdr(:,:,2) + (1.0/3)*hdr(:,:,3);
end

function n = step(edge, x)
n = double(x >= edge);
end

function n = pulse(edge0, edge1, x)
n = step(edge0, x) - step(edge1, x);
end

function n = pulseTrain(frequency, edge, x) %#ok<DEFNU>
% A pulse train: a signal that repeats with a given period, and is
% 0 when 0 <= mod(x/period,1) < edge, and 1 when mod(x/period,1) > edge.
% [Remember period = 1/fequency]
n = pulse(edge, 1, mod(frequency*x, 1));
end

function n = intfpt(edge, x)
% Integral of pulseTrain with period 1 from 0 to x
n = (1 - edge) .* floor(x) + max(x - floor(x) - edge, 0);
end

function n = filteredPulseTrain(frequency, edge, x, width)
% Filtered pulse train: it's not as simple as just returning the mod
% of filteredpulse -- you have to take into account that the filter may
% cover multiple pulses in the train.
% Strategy: consider the function INTFPT, which is the integral of the
% train from 0 to x. Just subtract!

% First, normalize so period == 1 and our domain of interest is > 0
w  = width * frequency;
x0 = x*frequency - (0.5 * w);
x1 = x0 + w;
% Now we want to integrate the normalized pulsetrain over [x0,x1]
n = (intfpt(edge,x1) - intfpt(edge,x0)) ./ (x1-x0);
end
