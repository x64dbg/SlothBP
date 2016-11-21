set RELEASEDIR=.\release
mkdir %RELEASEDIR%\plugins\x32
mkdir %RELEASEDIR%\plugins\x64
copy bin\x32\SlothBP.dp32 %RELEASEDIR%\plugins\x32\
copy SlothBP.ini %RELEASEDIR%\plugins\x32\
copy bin\x64\SlothBP.dp64 %RELEASEDIR%\plugins\x64\
copy SlothBP.ini %RELEASEDIR%\plugins\x64\