if "%ComputerName%" =="C6_V1_HEAD" (
	start S:\apps\dual_test\sculptup\bin\world_builder.exe configFile C:\Users\Admin2\Desktop\FionaConfigDual.txt --wb
) else (
	start S:\apps\dual_test\sculptup\bin\world_builder.exe configFile C:\Users\Admin2\Desktop\FionaConfigDP.txt windowX 0 windowY 0 windowW 1920 windowH 1920 navigationSpeed 0.01 rotationSpeed 0.007 --wb kevinOffset -0.033 -0.0266 0.041
	TIMEOUT 3
	start S:\apps\dual_test\sculptup\bin\world_builder.exe configFile C:\Users\Admin2\Desktop\FionaConfigDP.txt windowX 1920 windowY 0 windowW 1920 windowH 1920 navigationSpeed 0.01 rotationSpeed 0.007 --wb kevinOffset 0.033 -0.0266 0.041
)

