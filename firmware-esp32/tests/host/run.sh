#!/bin/sh
set -eu
cc -std=c11 -Wall -Wextra -Werror -I main tests/host/impact_detector_test.c main/impact_detector.c -lm -o /tmp/impact-test
/tmp/impact-test
cc -std=c11 -Wall -Wextra -Werror -DEVENT_LOG_HOST_TEST -I tests/host/include -I main tests/host/event_log_test.c -lm -o /tmp/event-log-test
/tmp/event-log-test

cc -std=c11 -Wall -Wextra -Werror -I tests/host/include -I main tests/host/mpu_fifo_test.c main/mpu6050.c -lm -o /tmp/mpu-fifo-test
/tmp/mpu-fifo-test
