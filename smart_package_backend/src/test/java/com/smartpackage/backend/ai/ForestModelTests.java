package com.smartpackage.backend.ai;
import java.util.*;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import tools.jackson.databind.json.JsonMapper;
class ForestModelTests {
    private ForestModel model(List<ForestModel.Node> tree,List<Double> vector,List<Double> expected) {
        return new ForestModel(1,"test","2026-10-06","test","test-device",MotionFeatures.NAMES,
            List.of("IMPACT","NORMAL","VIBRATION"),List.of(tree),.7,null,List.of(new ForestModel.Vector(vector,expected)));
    }
    private List<Double> vector(double peak) {var f=new ArrayList<>(Collections.nCopies(16,0.0));f.set(0,peak);return f;}
    private ForestModel.Node leaf(double a,double b,double c) {return new ForestModel.Node(-1,0,-1,-1,List.of(a,b,c));}
    @Test void belowThresholdIsUncertainRatherThanAFalseConclusion() {
        var m=model(List.of(leaf(.6,.3,.1)),vector(1),List.of(.6,.3,.1));m.validate();
        assertEquals("UNCERTAIN",m.predict(vector(1)).label());assertEquals("IMPACT",m.predict(vector(1)).candidate());
    }
    @Test void branchUsesFloat32LikePythonTraining() {
        var tree=List.of(new ForestModel.Node(0,1.00000001,1,2,List.of()),leaf(0,1,0),leaf(1,0,0));
        var m=model(tree,vector(1.00000002),List.of(0.0,1.0,0.0));m.validate();
        assertEquals("NORMAL",m.predict(vector(1.00000002)).label());
        assertEquals("IMPACT",m.predict(vector(1.000001)).label());
    }
    @Test void cyclesAndInconsistentValidationVectorsAreRejected() {
        var m=model(List.of(new ForestModel.Node(0,1,0,0,List.of())),vector(1),List.of(1.0,0.0,0.0));
        assertThrows(IllegalArgumentException.class,m::validate);
        var wrong=model(List.of(leaf(1,0,0)),vector(1),List.of(0.0,1.0,0.0));
        assertThrows(IllegalArgumentException.class,wrong::validate);
    }
    @Test void exportedPythonForestMatchesJavaButCannotBeActivatedAsRealData() throws Exception {
        try(var input=getClass().getResourceAsStream("/ai/synthetic-forest.json")) {
            assertNotNull(input);
            var m=JsonMapper.builder().build().readValue(input,ForestModel.class);
            m.validate();
            assertThrows(IllegalArgumentException.class,m::validateEvaluation);
        }
    }
}
