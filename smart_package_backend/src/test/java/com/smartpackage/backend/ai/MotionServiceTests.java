package com.smartpackage.backend.ai;
import java.util.*;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;
import tools.jackson.databind.json.JsonMapper;
import com.smartpackage.backend.repository.DeviceRepository;
import com.smartpackage.backend.entity.DeviceEntity;
class MotionServiceTests {
    private final AiCaptureRepository captures=mock(AiCaptureRepository.class);
    private final AiWindowRepository windows=mock(AiWindowRepository.class);
    private final AiModelService models=mock(AiModelService.class);
    private final DeviceRepository devices=mock(DeviceRepository.class);
    private final JsonMapper mapper=JsonMapper.builder().build();
    private final MotionService service=new MotionService(captures,windows,models,devices,mapper);
    private String payload(AiCapture c) {
        return mapper.writeValueAsString(new MotionWindow("test-device",1,"0123456789abcdef0123456789abcdef",
            1,c==null?"":c.id,1,0,"NORMAL",MotionFeaturesTests.samples()));
    }
    @Test void liveDataDoesNotPersistRawSamplesOrInventAnAiResult() {
        assertNull(service.accept("smart-package/test-device/motion-window",payload(null)));
        var result=(Map<?,?>)service.status("test-device");
        var live=(MotionService.Live)result.get("latest");
        assertEquals("NO_MODEL",live.status());assertNull(live.label());assertNull(live.score());
        verify(windows,never()).saveAndFlush(any());
    }
    @Test void persistenceFailureDoesNotProduceAReceipt() {
        var c=new AiCapture("test-device","capture","group",10);when(captures.findById(c.id)).thenReturn(Optional.of(c));
        when(windows.saveAndFlush(any())).thenThrow(new IllegalStateException("database failed"));
        assertThrows(IllegalStateException.class,()->service.accept("smart-package/test-device/motion-window",payload(c)));
    }
    @Test void completedCaptureAllowsNextCaptureAndDuplicateIsNotInsertedTwice() {
        var c=new AiCapture("test-device","capture","group",10);when(captures.findById(c.id)).thenReturn(Optional.of(c));
        when(windows.countByCapture_Id(c.id)).thenReturn(5L);
        assertTrue(service.accept("smart-package/test-device/motion-window",payload(c)).contains("SAVED"));
        assertTrue(c.stopped);
        when(windows.existsByCapture_IdAndSequence(c.id,1)).thenReturn(true);
        assertTrue(service.accept("smart-package/test-device/motion-window",payload(c)).contains("SAVED"));
        verify(windows,times(1)).saveAndFlush(any());
        when(devices.findForCapture("test-device")).thenReturn(Optional.of(new DeviceEntity("test-device")));
        when(captures.saveAndFlush(any())).thenAnswer(i->i.getArgument(0));
        assertNotNull(service.create("test-device","next","group",10));
    }
    @Test void manualLabelsAreRestrictedAndDeviceOwnershipIsChecked() {
        var c=new AiCapture("test-device","capture","group",10);
        var w=new AiWindowEntity();w.capture=c;when(windows.findById(1L)).thenReturn(Optional.of(w));
        assertThrows(IllegalArgumentException.class,()->service.label("test-device",1,"DROP"));
        assertThrows(org.springframework.web.server.ResponseStatusException.class,()->service.label("other-device",1,"IMPACT"));
        service.label("test-device",1,"IMPACT");assertEquals("IMPACT",w.label);
    }
}
