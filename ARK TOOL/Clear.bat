@echo Off  
del /s /a *.exe *.suo *.ncb *.db *.user *.pdb *.netmodule *.aps *.ilk *.sdf 2>nul 

FOR /R . %%d IN (.) DO rd /s /q "%%d\.vs" 2>nul  
FOR /R . %%d IN (.) DO rd /s /q "%%d\Debug" 2>nul  
FOR /R . %%d IN (.) DO rd /s /q "%%d\Release" 2>nul  
FOR /R . %%d IN (.) DO rd /s /q "%%d\ipch" 2>nul  
FOR /R . %%d IN (.) DO rd /s /q "%%d\x64" 2>nul  

  
rem ���Properties�ļ���Ϊ�գ���ɾ����  
rem FOR /R . %%d in (.) do rd /q "%%d\Properties" 2> nul 