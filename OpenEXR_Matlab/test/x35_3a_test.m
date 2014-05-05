% Brief script to test the new exrinfo and exrreadchannels

% Test multichannel OpenEXR file from fnord software:
%  URL:  http://www.fnordware.com/OpenEXR/x35_3a.exr
%  SHA1: bfbb4573fb6b93e6f8074d914202e51a75d608b1
%  MD5:  901a11ad9d9113b1f28265abcd51bd64

% Assume the file is in the current directory
fname = 'x35_3a.exr';

finfo = exrinfo(fname);

assert(all(size(finfo.channels)==[1 49]), 'Unexpected number of channels!')

% Test some channels
cExpected = {'Background.G' 'Caustics.G' 'Diffuse.B' 'Light.R'};
cActual   = {finfo.channels{7} finfo.channels{10} ...
    finfo.channels{12} finfo.channels{21}};
assert(all(strcmp(cExpected, cActual)), 'Unexpected channel names!');

% Image size
assert(all(finfo.size == [384 547]), 'Unexpected image size!');

% Check some attributes
attr = finfo.attributes;
assert(strcmp(attr('compression'), 'zips'), 'Unexpected compression!');
dw=attr('dataWindow');
assert(all([dw.min dw.max] == [70 0 616 383]), 'Unexpected data window!');


% Check reading some channels through both interfaces
imgRGB = exrread(fname);
[iR, iG] = exrreadchannels(fname, 'R', 'G');
iB = exrreadchannels(fname, 'B');
assert(all(all(imgRGB(:,:,1) == iR)), 'Unexpected R channel!');
assert(all(all(imgRGB(:,:,2) == iG)), 'Unexpected G channel!');
assert(all(all(imgRGB(:,:,3) == iB)), 'Unexpected B channel!');

cNames = {'Atmosphere.G' 'Velocity.Y' 'realcolor.B' 'RawGI.R' 'depth.Z'};
cData = exrreadchannels(fname, cNames);
vyData = exrreadchannels(fname, 'Velocity.Y');
assert(all(all(vyData == cData('Velocity.Y'))), ...
    'Unexpected Velocity.Y channel data!');

assert(int32(sum(sum(cData('RawGI.R')))) == 15970, ...
    'Unexpected RawGI.R channel data!');

% Assemble an image with some channels
giChannels = {'RawGI.R' 'RawGI.G' 'RawGI.B'};
[giR, giG, giB] = exrreadchannels(fname, giChannels);
giImg(:,:,1) = giR;
giImg(:,:,2) = giG;
giImg(:,:,3) = giB;

f = figure('Name', 'OpenEXR test, close figure to continue');
subplot(2,2,1); imshow(imgRGB); title('RGB');
subplot(2,2,2); imshow(giImg); title('RawGI Layer');
subplot(2,2,3); imagesc(cData('Velocity.Y')); axis image; axis off; title('Velocity.Y'); colorbar;
subplot(2,2,4); imagesc(cData('depth.Z')); axis image; axis off; title('depth.Z'); colorbar;
drawnow     % Necessary to print the message
waitfor(f);



disp('All OpenEXR tests were successful.')
