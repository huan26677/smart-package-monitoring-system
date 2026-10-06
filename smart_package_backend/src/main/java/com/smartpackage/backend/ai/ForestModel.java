package com.smartpackage.backend.ai;
import java.util.*;

public record ForestModel(int schemaVersion,String version,String trainedAt,String sklearnVersion,String deviceId,
        List<String> featureNames,List<String> classes,List<List<Node>> trees,double scoreThreshold,
        Evaluation evaluation,List<Vector> validationVectors) {
    public static final Set<String> LABELS=Set.of("NORMAL","VIBRATION","IMPACT");
    public record Node(int feature,double threshold,int left,int right,List<Double> probabilities) {}
    public record ClassMetrics(double precision,double recall,double f1,int support) {}
    public record Metrics(double accuracy,double macroF1,double coverage,Map<String,ClassMetrics> perClass,List<List<Integer>> confusion) {}
    public record Evaluation(String source,boolean groupSplit,List<String> trainGroups,List<String> testGroups,
            int trainCount,int testCount,Map<String,Integer> labelCounts,Map<String,Integer> groupsPerClass,Metrics ai,Metrics baseline) {}
    public record Vector(List<Double> features,List<Double> probabilities) {}
    public record Prediction(String label,String candidate,double score) {}
    public void validate() {
        require(schemaVersion==1 && featureNames!=null && featureNames.equals(MotionFeatures.NAMES),"Phiên bản đặc trưng không tương thích.");
        require(version!=null && version.matches("[A-Za-z0-9_-]{1,80}"),"Tên phiên bản mô hình không hợp lệ.");
        require(deviceId!=null && deviceId.matches("[A-Za-z0-9_-]{1,64}"),"Mã thiết bị mô hình không hợp lệ.");
        require(classes!=null && classes.size()==3 && new HashSet<>(classes).equals(LABELS),"Mô hình cần đúng ba nhóm đã quy định.");
        require(Double.isFinite(scoreThreshold) && scoreThreshold>=0.5 && scoreThreshold<=0.95,"Ngưỡng điểm dự đoán không hợp lệ.");
        require(trees!=null && !trees.isEmpty() && trees.size()<=64,"Mô hình cần 1–64 cây.");
        for(var tree:trees) {
            require(tree!=null && !tree.isEmpty() && tree.size()<=127,"Cây vượt giới hạn kích thước.");
            boolean[] seen=new boolean[tree.size()];checkNode(tree,0,0,seen);
            for(boolean v:seen) require(v,"Cây chứa nút không thể tới.");
        }
        require(validationVectors!=null && !validationVectors.isEmpty() && validationVectors.size()<=20,"Thiếu mẫu kiểm tra tính tương thích mô hình.");
        for(var v:validationVectors) {
            require(v!=null && v.probabilities()!=null && v.probabilities().size()==3,"Mẫu kiểm tra không hợp lệ.");
            var actual=probabilities(v.features());
            for(int i=0;i<3;i++) require(v.probabilities().get(i)!=null && Double.isFinite(v.probabilities().get(i))
                    && Math.abs(actual[i]-v.probabilities().get(i))<0.000001,"Kết quả Java khác công cụ huấn luyện.");
        }
    }
    private void checkNode(List<Node> tree,int index,int depth,boolean[] seen) {
        require(index>=0 && index<tree.size() && depth<=6 && !seen[index],"Cây có chu trình, nút dùng chung hoặc vượt độ sâu.");
        seen[index]=true;var node=tree.get(index);require(node!=null,"Nút cây trống.");
        if(node.feature()==-1) {
            require(node.left()==-1 && node.right()==-1 && node.probabilities()!=null && node.probabilities().size()==3,"Nút lá không hợp lệ.");
            double sum=0;
            for(Double value:node.probabilities()) {require(value!=null && Double.isFinite(value) && value>=0 && value<=1,"Trọng số lá không hợp lệ.");sum+=value;}
            require(Math.abs(sum-1)<0.000001,"Tổng trọng số lá phải bằng một.");
        } else {
            require(node.feature()>=0 && node.feature()<MotionFeatures.NAMES.size() && Double.isFinite(node.threshold()),"Điều kiện cây không hợp lệ.");
            checkNode(tree,node.left(),depth+1,seen);checkNode(tree,node.right(),depth+1,seen);
        }
    }
    public void validateEvaluation() {
        var e=evaluation;
        require(e!=null && "real-device".equals(e.source()) && e.groupSplit(),"Chỉ nhận mô hình đánh giá từ mẫu thật, tách theo nhóm buổi thử.");
        require(e.trainGroups()!=null && e.testGroups()!=null && !e.trainGroups().isEmpty() && !e.testGroups().isEmpty()
                && Collections.disjoint(e.trainGroups(),e.testGroups()),"Nhóm huấn luyện và kiểm tra bị trùng.");
        require(e.trainCount()>0 && e.testCount()>0 && e.labelCounts()!=null && e.groupsPerClass()!=null,"Thiếu thông tin bộ dữ liệu.");
        for(String label:LABELS) require(e.labelCounts().getOrDefault(label,0)>=30 && e.groupsPerClass().getOrDefault(label,0)>=3,"Mỗi nhóm cần ít nhất 30 đoạn hợp lệ trong ba nhóm buổi thử.");
        validateMetrics(e.ai());validateMetrics(e.baseline());
    }
    private static void validateMetrics(Metrics m) {
        require(m!=null,"Thiếu chỉ số đánh giá.");
        for(double v:new double[]{m.accuracy(),m.macroF1(),m.coverage()}) require(Double.isFinite(v) && v>=0 && v<=1,"Chỉ số đánh giá ngoài khoảng cho phép.");
        require(m.perClass()!=null && m.perClass().keySet().containsAll(LABELS) && m.confusion()!=null && m.confusion().size()==4,"Thiếu đánh giá từng loại.");
        for(var c:m.perClass().values()) {require(c!=null && c.support()>=0,"Số mẫu kiểm tra không hợp lệ.");for(double v:new double[]{c.precision(),c.recall(),c.f1()}) require(Double.isFinite(v)&&v>=0&&v<=1,"Chỉ số từng loại không hợp lệ.");}
        for(var row:m.confusion()) require(row!=null && row.size()==4 && row.stream().allMatch(v->v!=null&&v>=0),"Bảng nhầm lẫn không hợp lệ.");
    }
    public double[] probabilities(List<Double> features) {
        require(features!=null && features.size()==featureNames.size() && features.stream().allMatch(v->v!=null&&Double.isFinite(v)),"Đặc trưng dự đoán không hợp lệ.");
        double[] result=new double[classes.size()];
        for(var tree:trees) {
            int index=0;
            while(tree.get(index).feature()!=-1) {
                var node=tree.get(index);
                // scikit-learn converts input features to float32 before comparing thresholds.
                index=(float)(double)features.get(node.feature())<=node.threshold()?node.left():node.right();
            }
            for(int i=0;i<result.length;i++) result[i]+=tree.get(index).probabilities().get(i)/trees.size();
        }
        return result;
    }
    public Prediction predict(List<Double> features) {
        var probs=probabilities(features);int best=0;
        for(int i=1;i<probs.length;i++) if(probs[i]>probs[best]) best=i;
        return new Prediction(probs[best]>=scoreThreshold?classes.get(best):"UNCERTAIN",classes.get(best),probs[best]);
    }
    private static void require(boolean valid,String message) {if(!valid) throw new IllegalArgumentException(message);}
}
