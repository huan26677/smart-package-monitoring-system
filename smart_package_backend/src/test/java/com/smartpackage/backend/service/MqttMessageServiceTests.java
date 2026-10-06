package com.smartpackage.backend.service;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;
import org.junit.jupiter.api.Test;
import tools.jackson.databind.json.JsonMapper;
import com.smartpackage.backend.dto.PackageEventMessage;

class MqttMessageServiceTests {
    private static final String PAYLOAD = """
        {"deviceId":"esp32-001","eventId":1,"type":"IMPACT","level":"STRONG",
         "g":8.0,"angle":0,"vibration":0.1,"uptimeMs":1000,"timestamp":1700000000,
         "durationMs":30,"saturated":false}
        """;
    @Test
    void committedAndDuplicateEventsBothReceiveConfirmation() {
        var persistence = mock(MqttPersistenceService.class);
        when(persistence.saveEvent(any())).thenReturn(true, false);
        var service = new MqttMessageService(JsonMapper.builder().build(), persistence, mock(WifiAnchorService.class));
        assertEquals(new MqttMessageService.EventReceipt("esp32-001",1), service.handle("smart-package/esp32-001/event",PAYLOAD));
        assertEquals(new MqttMessageService.EventReceipt("esp32-001",1), service.handle("smart-package/esp32-001/event",PAYLOAD));
    }
    @Test
    void databaseFailureDoesNotConfirmEvent() {
        var persistence = mock(MqttPersistenceService.class);
        when(persistence.saveEvent(any())).thenThrow(new IllegalStateException("database unavailable"));
        var service = new MqttMessageService(JsonMapper.builder().build(), persistence, mock(WifiAnchorService.class));
        assertNull(service.handle("smart-package/esp32-001/event",PAYLOAD));
    }
    @Test
    void mismatchedTopicAndInvalidMeasurementAreRejected() {
        var persistence = mock(MqttPersistenceService.class);
        var service = new MqttMessageService(JsonMapper.builder().build(), persistence, mock(WifiAnchorService.class));
        assertNull(service.handle("smart-package/another-device/event",PAYLOAD));
        assertNull(service.handle("smart-package/esp32-001/event",PAYLOAD.replace("\"g\":8.0", "\"g\":-1.0")));
        verifyNoInteractions(persistence);
    }
    @Test
    void previousFirmwarePayloadStillWorksWithoutNewFields() {
        var persistence = mock(MqttPersistenceService.class);
        var service = new MqttMessageService(JsonMapper.builder().build(), persistence, mock(WifiAnchorService.class));
        String old = PAYLOAD.replace(",\n \"durationMs\":30,\"saturated\":false", "");
        var mapper = JsonMapper.builder().build();
        PackageEventMessage parsed = mapper.readValue(old, PackageEventMessage.class);
        assertNotNull(service.handle("smart-package/esp32-001/event",old));
        assertEquals(1,parsed.eventId());
        assertNull(parsed.durationMs());
        assertNull(parsed.saturated());
    }
}
