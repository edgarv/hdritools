package edu.cornell.graphics.exr;

import org.junit.runner.RunWith;
import org.junit.runners.Suite;

@RunWith(Suite.class)
@Suite.SuiteClasses({
    edu.cornell.graphics.exr.EXRSimpleImageTest.class,
    edu.cornell.graphics.exr.HalfTest.class,
    edu.cornell.graphics.exr.io.InputFileInfoBasicTest.class,
    edu.cornell.graphics.exr.HeaderBasicTest.class,
})
public class AllTests {
    // Nothing to be done
}
