% Brief script to test the new exrinfo and exrreadchannels

% Test multichannel OpenEXR file from fnord software:
%  URL:  http://www.fnordware.com/OpenEXR/x35_3a.exr
%  SHA1: bfbb4573fb6b93e6f8074d914202e51a75d608b1
%  MD5:  901a11ad9d9113b1f28265abcd51bd64

% Assume the file is in the current directory
fname = 'x35_3a.exr';

finfo = exrinfo(fname);

if ~all(size(finfo.channels) == [1 49])
   fprintf(2, 'Unexpected number of channels!\n');
   return
end

% Test some channels
cExpected = {'Background.G' 'Caustics.G' 'Diffuse.B' 'Light.R'};
cActual   = {finfo.channels{7} finfo.channels{10} ...
    finfo.channels{12} finfo.channels{21}};
if ~all(strcmp(cExpected, cActual))
   fprintf(2, 'Unexpected channel names!\n');
   return
end

% Image size
if ~all(finfo.size == [384 547])
   fprintf(2, 'Unexpected image size!\n');
   return
end

% Check some attributes
attr = finfo.attributes;
if ~strcmp(attr('compression'), 'zips')
   fprintf(2, 'Unexpected compression!\n');
   return
end
dw=attr('dataWindow');
if ~all([dw.min dw.max] == [70 0 616 383])
   fprintf(2, 'Unexpected data window!\n');
   return 
end


% Check reading some channels through both interfaces
imgRGB = exrread(fname);
[iR, iG] = exrreadchannels(fname, 'R', 'G');
iB = exrreadchannels(fname, 'B');
if ~all(all(imgRGB(:,:,1) == iR))
   fprintf(2, 'Unexpected R channel!\n');
   return 
end
if ~all(all(imgRGB(:,:,2) == iG))
   fprintf(2, 'Unexpected G channel!\n');
   return 
end
if ~all(all(imgRGB(:,:,3) == iB))
   fprintf(2, 'Unexpected B channel!\n');
   return 
end

cNames = {'Atmosphere.G' 'Velocity.Y' 'realcolor.B' 'RawGI.R' 'depth.Z'};
cData = exrreadchannels(fname, cNames);
vyData = exrreadchannels(fname, 'Velocity.Y');
if ~all(all(vyData == cData('Velocity.Y')))
   fprintf(2, 'Unexpected Velocity.Y channel data!\n');
   return
end

if int32(sum(sum(cData('RawGI.R')))) ~= 15970
   fprintf(2, 'Unexpected RawGI.R channel data!\n');
   return   
end

% Assemble an image with some channels
giChannels = {'RawGI.R' 'RawGI.G' 'RawGI.B'};
[giR, giG, giB] = exrreadchannels(fname, giChannels);
giImg(:,:,1) = giR;
giImg(:,:,2) = giG;
giImg(:,:,3) = giB;

subplot(2,2,1); imshow(imgRGB); title('RGB');
subplot(2,2,2); imshow(giImg); title('RawGI Layer');
subplot(2,2,3); imagesc(cData('Velocity.Y')); axis image; axis off; title('Velocity.Y'); colorbar;
subplot(2,2,4); imagesc(cData('depth.Z')); axis image; axis off; title('depth.Z'); colorbar;



fprintf('All tests were successful.\n')
