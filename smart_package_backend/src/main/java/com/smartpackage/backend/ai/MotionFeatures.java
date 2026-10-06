package com.smartpackage.backend.ai;
import java.util.List;
public final class MotionFeatures {
    public static final List<String> NAMES=List.of("peak_g","min_g","mean_g","std_g","rms_dynamic_g",
        "gyro_rms_dps","gyro_peak_dps","jerk_peak_gps","above_impact_ms","below_freefall_ms",
        "ax_std","ay_std","az_std","window_ms","sample_period_ms","saturated_ratio");
    public record Result(List<Double> values,boolean timingValid,boolean saturated) {}
    private MotionFeatures() {}
    public static Result extract(List<List<Double>> samples) {
        if(samples==null || samples.size()!=200) throw new IllegalArgumentException("Đoạn đo phải có đúng 200 mẫu.");
        double peak=0,min=Double.MAX_VALUE,sum=0,sum2=0,dyn2=0,gyro2=0,gyroPeak=0,jerk=0,high=0,low=0,previousG=0,maxDt=0;
        int clipped=0,n=samples.size();double[] axes=new double[3],axis2=new double[3];
        for(int i=0;i<n;i++) {
            var row=samples.get(i);
            if(row==null || row.size()!=7 || row.stream().anyMatch(v->v==null || !Double.isFinite(v)))
                throw new IllegalArgumentException("Mẫu đo phải chứa thời gian và sáu giá trị cảm biến hữu hạn.");
            if(row.get(0)<0 || row.get(0)!=Math.rint(row.get(0))) throw new IllegalArgumentException("Thời gian mẫu không hợp lệ.");
            for(int a=1;a<=3;a++) if(Math.abs(row.get(a))>17) throw new IllegalArgumentException("Gia tốc vượt định dạng đo.");
            for(int a=4;a<=6;a++) if(Math.abs(row.get(a))>2500) throw new IllegalArgumentException("Tốc độ góc vượt định dạng đo.");
            double dt=i==0?10:row.get(0)-samples.get(i-1).get(0);
            if(dt<=0 || dt>500) throw new IllegalArgumentException("Thứ tự hoặc khoảng cách mẫu không hợp lệ.");
            maxDt=Math.max(maxDt,dt);
            double g=Math.sqrt(row.get(1)*row.get(1)+row.get(2)*row.get(2)+row.get(3)*row.get(3));
            double rot=Math.sqrt(row.get(4)*row.get(4)+row.get(5)*row.get(5)+row.get(6)*row.get(6));
            peak=Math.max(peak,g);min=Math.min(min,g);sum+=g;sum2+=g*g;dyn2+=(g-1)*(g-1);gyro2+=rot*rot;gyroPeak=Math.max(gyroPeak,rot);
            if(i>0) jerk=Math.max(jerk,Math.abs(g-previousG)*1000/dt);previousG=g;
            if(g>=2.5) high+=dt;if(g<0.35) low+=dt;
            boolean saturated=false;
            for(int a=0;a<3;a++) {double v=row.get(a+1);axes[a]+=v;axis2[a]+=v*v;saturated|=Math.abs(v)>=15.9;}
            for(int a=4;a<=6;a++) saturated|=Math.abs(row.get(a))>=495;
            if(saturated) clipped++;
        }
        double duration=samples.get(n-1).get(0)-samples.get(0).get(0)+10,period=(duration-10)/(n-1);
        var values=List.of(peak,min,sum/n,Math.sqrt(Math.max(0,sum2/n-(sum/n)*(sum/n))),Math.sqrt(dyn2/n),
            Math.sqrt(gyro2/n),gyroPeak,jerk,high,low,
            Math.sqrt(Math.max(0,axis2[0]/n-axes[0]*axes[0]/n/n)),Math.sqrt(Math.max(0,axis2[1]/n-axes[1]*axes[1]/n/n)),
            Math.sqrt(Math.max(0,axis2[2]/n-axes[2]*axes[2]/n/n)),duration,period,(double)clipped/n);
        return new Result(values,maxDt<=30 && duration>=1750 && duration<=2500,clipped>0);
    }
}
