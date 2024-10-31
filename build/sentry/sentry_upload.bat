sentry-cli-Windows-x86_64.exe upload-dif -o sdk13-sentry -p tf2c-native-proj ..\..\game\ --log-level=info || echo "error %errorlevel%" && exit /b %errorlevel%
pause