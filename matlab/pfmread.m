function img = pfmread(filename)
%PFMREAD    Read Portable Float Map image.
%   HDR = PFMREAD(FILENAME) reads the high dynamic range image HDR from
%   FILENAME, which points to a Portable Float Map .pfm file.  
%   HDR is an m-by-n-by-3 RGB array in the range (-inf,inf) and has type 
%   single.  For scene-referred datasets, these values usually are scene 
%   illumination in radiance units.  To display these images, use an 
%   appropriate tone-mapping operator.
%
%   Class Support
%   -------------
%   The output image HDR is an m-by-n-by-3 image with type single.
%
%   Example
%   -------
%       hdr = pfmread('office.pfm');
%       rgb = tonemap(hdr);
%       imshow(rgb);
%
%   Reference: "PFM Portable FloatMap Image Format" by Paul Debevec
%              http://www.pauldebevec.com/Research/HDR/PFM/
%              (Accessed July 2013.)
%
%   See also TONEMAP.

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

fid = openFile(filename);
info = readHeader(fid);
img = readImage(fid, info);

end



function r = condexpr(cond, trueVal, falseVal)
%condexpr   Workaround for (cond ? trueVal : falseVal)
if (cond)
    r = trueVal;
else
    r = falseVal;
end
end


function fid = openFile(filename)
%openFile   Open a file.

iptcheckinput(filename, {'char'}, {'row'}, mfilename, 'FILENAME', 1);

[fid, msg] = fopen(filename, 'r');
if (fid == -1)
    error('images:pfmread:fileOpen', ...
          'Unable to open file "%s" for reading: %s.', filename, msg);    
end

end


function fileInfo = readHeader(fid)
%readHeader   Extracts the header information

% Ensure that we're reading a PFM file.
magic = fscanf(fid, '%c%c%c', 3);
if (isempty(regexp(magic, '^P[Ff]\s$', 'ONCE')))
    fclose(fid);
    error('images:pfmread:notPFM', 'Not a PFM file.')
end

fileInfo.isColor = magic(2) == 'F';

lineStart = fread(fid, 1, 'uint8');
while (lineStart == '#')
    fgetl(fid); % Remove the rest of the line
    lineStart = fread(fid, 1, 'uint8');
end

% Move back to read again the last byte
if (fseek(fid, -1, 'cof') ~= 0)
   fclose(fid);
   error('images:pfmread:ioerror', 'Could not reposition after comments')
end

% Read the dimensions and the endianness. The last character is actually
% ignored as well, but if it is %*c it won't be read.
data = fscanf(fid, '%d%*c%d%*c%f%c', 4);
if (isempty(data) || size(data,1) ~= 4)
    fclose(fid);
    error('images:pfmread:badHeader', ...
        'Could not read the image size and endianness')
end

% Assign the info
fileInfo.width = data(1);
fileInfo.height = data(2);
fileInfo.isLittleEndian = data(3) <= 0;

end


function img = readImage(fid, info)
%readImage   Get the decoded RGB data from the file.

% The file pointer (fid) should be at the start of the image data.

% Allocate space for the output.
img(info.height, info.width, 3) = single(0);

% Read the remaining data
pixelsLin = fread(fid, inf, '*float32', ...
    condexpr(info.isLittleEndian, 'ieee-le', 'ieee-be'));
fclose(fid);

numBands = condexpr(info.isColor, 3, 1);
if (size(pixelsLin,1) ~= info.width * info.height * numBands)
    error('images:pfmread:badData', ...
        'The image does not have the right amount of pixels')
end

% Rearrange the data from interleaved, row-major order (raster style)
% to planar, column-major order (Matlab style.)
pixels = permute(reshape(pixelsLin, ...
    [numBands, info.width, info.height]), [3 2 1]);

% PFM stores its scanlines bottom-up. Fix it and clone the channels for B&W.
if (info.isColor)
    img(:,:,:) = pixels(end:-1:1,:,:);
else
    img(:,:,:) = pixels(end:-1:1,:,[1 1 1]);
end

end
