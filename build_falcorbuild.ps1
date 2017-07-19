



$configuration = "ReleaseVK"
$target = "build"

$build_solution_target = "Falcor.sln"
$build_verbosity = " /v:normal "
$build_parallel = " /m "
$build_configuration = " /p:Configuration=${configuration}"
$build_target = "/target:${target}"

$msbuild = "msbuild.exe"
$args_escaper = "--%"

Write-Output "Starting Build with Configuration : ${configuration} ."

& $msbuild $args_escaper $build_solution_target $build_configuration $build_target $build_verbosity $build_parallel

if($?)
{
    Write-Output "Build Success!"
}
else
{
    throw "Build Failed!"
}

