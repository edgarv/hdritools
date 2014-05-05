% Simple bootstraper for the tests

function launch_test(mexpath)
    disp('OpenEXR MEX test')
    fprintf('Adding to the path \"%s\"\n', mexpath)
    addpath(mexpath)
    addpath('@test_dir@')
    tic
    try
      @test_script@
    catch ME
      fprintf(2, getReport(ME))
    end
    toc
    exit
end
