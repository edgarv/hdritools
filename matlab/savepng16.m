function savepng16(hdr, filename)
%SAVEPNG16(HDR, FILENAME) Saves a 16-bit sRGB PNG from the tone mapped hdr

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

% Convert each value to sRGB. This takes 16s on an 800x600 image.
% ldr = arrayfun(@srgb, min(1,max(hdr,0)));

% Calling array fun is just too expensive, do it the matrix way.
% The matrix way takes 2s on an 800x600 image.
ldr  = min(1,max(hdr,0));
mask = ldr <= 0.0031308;
a    = 0.055;
ldr  = 12.92*(ldr.*mask) + (1+a)*((ldr.*(1-mask)).^(1/2.4)) - a;

im = uint16(round(65535 * ldr));
imwrite(im, filename);

end


function s=srgb(c) %#ok<DEFNU>
% Converts the linear color C to the sRGB space
if c <= 0.0031308
    s = 12.92*c; 
else
    a = 0.055;
    s = (1+a)*c^(1/2.4) - a;
end
end