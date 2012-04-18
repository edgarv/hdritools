% Simple bootstraper for the tests

function launch_test(mexpath)
    fprintf('OpenEXR MEX test\n')
    fprintf('Adding to the path \"%s\"\n', mexpath)
    addpath(mexpath)
    addpath('@test_dir@')
    tic
    @test_script@
    toc
end
