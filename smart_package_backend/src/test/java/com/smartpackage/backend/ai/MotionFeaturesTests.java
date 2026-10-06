package com.smartpackage.backend.ai;
import java.util.*;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class MotionFeaturesTests {
    static List<List<Double>> samples() {
        var rows=new ArrayList<List<Double>>();
        for(int i=0;i<200;i++) rows.add(new ArrayList<>(List.of(i*10.0,0.0,0.0,1.0,0.0,0.0,0.0)));
        return rows;
    }
    @Test void restingSensorHasOneGAndNoDynamicMotion() {
        var r=MotionFeatures.extract(samples());
        assertTrue(r.timingValid());assertFalse(r.saturated());
        assertEquals(1,r.values().get(0));assertEquals(0,r.values().get(4));
        assertEquals(2000,r.values().get(13));assertEquals(10,r.values().get(14));
    }
    @Test void shortImpactAndRotationRemainVisibleInWindow() {
        var rows=samples();rows.get(50).set(3,8.0);rows.get(50).set(4,100.0);
        var r=MotionFeatures.extract(rows);
        assertEquals(8,r.values().get(0));assertEquals(700,r.values().get(7));
        assertEquals(10,r.values().get(8));assertEquals(100,r.values().get(6));
    }
    @Test void saturationAndTimingGapsAreFlagged() {
        var rows=samples();rows.get(50).set(1,16.0);
        assertTrue(MotionFeatures.extract(rows).saturated());
        rows=samples();rows.get(50).set(4,499.0);
        assertTrue(MotionFeatures.extract(rows).saturated());
        rows=samples();for(int i=100;i<200;i++) rows.get(i).set(0,rows.get(i).get(0)+40);
        assertFalse(MotionFeatures.extract(rows).timingValid());
    }
    @Test void malformedNonfiniteAndReorderedSamplesAreRejected() {
        assertThrows(IllegalArgumentException.class,()->MotionFeatures.extract(samples().subList(0,199)));
        var rows=samples();rows.get(10).set(0,rows.get(9).get(0));
        assertThrows(IllegalArgumentException.class,()->MotionFeatures.extract(rows));
        rows.get(10).set(0,100.0);rows.get(10).set(2,Double.NaN);
        assertThrows(IllegalArgumentException.class,()->MotionFeatures.extract(rows));
    }
}
