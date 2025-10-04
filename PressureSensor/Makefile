# Upload the data folder to the microcontroller
upload_fs:
	pio run -t uploadfs

compile:
	pio run

upload_firmware:
	pio run -t upload

monitor:
	pio device monitor

code_completion:
	pio run -t compiledb

release: upload_fs upload_firmware
