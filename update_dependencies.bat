@if not exist Framework\Externals mkdir Framework\Externals 
@call "%~dp0packman\packman.cmd" pull "%~dp0dependencies.xml"