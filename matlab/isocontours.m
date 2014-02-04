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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function M = dFdx(A)
%dFdx per-pixel first derivative in the x direction of the real 2D image A.
%   M = dFdx(A) approximates the  per pixel-first derivative of the
%   real 2D image A in the direction of x (columns in matrix terms.)
%
%   The function uses center differences of the interpolated values except
%   at the top and bottom edges, where it falls back to forward and
%   backwards differences respectively.

[m,n] = size(A);
M = zeros(m,n);

% Matlab-style code, working on entire columns. The code uses:
%  * Forward difference in the left edge
%  * Center difference using the interpolated values f(x-0.5),f(x+0.5)
%  * Backward difference in the right edge

M(:,1) = A(:,2) - A(:,1);
x = 2:n-1;
M(:,x) = 0.5 * (A(:, x+1) - A(:, x-1));
M(:,n) = A(:,n) - A(:, n-1);

% Reference C-style code
%{
for y=1:m
    % Forward difference in the left edge
    M(y,1) = A(y,2) - A(y,1);
    % Center difference using the interpolated values f(x-0.5),f(x+0.5)
    for x=2:n-1
        M(y,x) = 0.5 * (A(y, x+1) - A(y, x-1));
    end
    % Backward difference in the right edge
    M(y,n) = A(y,n) - A(y, n-1);
end
%}

end



% This is the sparse matrix version. It is slower for a single matrix,
% however it becomes quite fast if the scaling matrix 'T' can be reused
% between invocations.
% The input matrix X has to be of type 'double', otherwise the 
% sparse matrix multiply fails.
%{
function Gx = dFdx_matrix(X)

[m,n] = size(X);
i = (1:m*n);
j0 = [1:m, 1:(n-1)*m];
j1 = [(1:(n-1)*m) + m, (n-1)*m+1:m*n];
s  = [-ones(1,m) -0.5*ones(1, (n-2)*m) -ones(1,m)];
T = sparse([i,i]', [j0,j1]', [s,-s]');

Gx = reshape(T*X(:), m, n);

end
%}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function M = dFdy(A)
%dFdy per-pixel first derivative in the y direction of the real 2D image A.
%   M = dFdy(A) approximates the  per pixel-first derivative of the
%   real 2D image A in the direction of y (rows in matrix terms.)
%
%   The function uses center differences of the interpolated values except
%   at the top and bottom edges, where it falls back to forward and
%   backwards differences respectively.

[m,n] = size(A);
M = zeros(m,n);

% Matlab-style code, working on entire rows. The code uses:
%  * Forward difference in the top edge
%  * Center difference using the interpolated values f(x-0.5),f(x+0.5)
%  * Backward difference in the bottom edge

M(1,:) = A(2,:) - A(1,:);
y = 2:m-1;
M(y,:) = 0.5 * (A(y+1,:) - A(y-1,:));
M(m,:) = A(m,:) - A(m-1,:);

% Reference C-style code
%{
for x=1:n
    % Forward difference in the top edge
    M(1,x) = A(2,x) - A(1,x);
    % Center difference using the interpolated values f(x-0.5),f(x+0.5)
    for y=2:m-1
        M(y,x) = 0.5 * (A(y+1,x) - A(y-1,x));
    end
    % Backward difference in the bottom edge
    M(m,x) = A(m,x) - A(m-1,x);
end
%}

end



% This is the sparse matrix version. It is slower for a single matrix,
% however it becomes quite fast if the scaling matrix 'T' can be reused
% between invocations.
% The input matrix X has to be of type 'double', otherwise the 
% sparse matrix multiply fails.
%{
function Gy = dFdy_matrix(X)

[m,n] = size(X);
i = (1:m*n)';
Offsets = m*ones(m,1)*(0:n-1);
j0 = [1 1:m-1]' * ones(1,n) + Offsets;
j1 = [2:m m]'   * ones(1,n) + Offsets;
s  = [-1; -0.5 * ones(m-2, 1); -1] * ones(1,m);
T = sparse([i;i], [j0(:); j1(:)], [s(:); -s(:)]);

Gy = reshape(T*X(:), m, n);

end
%}
