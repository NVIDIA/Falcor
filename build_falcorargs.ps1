[CmdletBinding()]


<#

.SYNOPSIS
Parameterized Build for Falcor.

.DESCRIPTION
Builds Falcor based on the Arguments provided.

.EXAMPLE
./BuildFalcor.ps1 ../Falcor.sln Release

.NOTES
Put some notes here.

#>


param
(   
    [string]$argTargetSolution = "Falcor.sln"

    [string]$argBuildConfiguration = "ReleaseD3D12",

    [string]$argBuildType = "rebuild"

    [string]$argBuildVerbosity = "normal"

)



$build_solution_target = $argTargetSolution.ToLower()
$build_verbosity = " /v:${argBuildVerbosity.ToLower()} "
$build_parallel = " /m "
$build_configuration = " /p:Configuration=${argBuildConfiguration}"
$build_target = "/target:${argBuildType}"

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

