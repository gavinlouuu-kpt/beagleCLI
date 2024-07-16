Issue:
2024/07/16
- ADS detail function crashes ESP32 after several samples being acquired
- ADS continuous sampling when put into sampling loop the notification seems to be misread and lost. Causing second sample reading to go on indefinitely. No stop signal. Weirdly long pause between trials.