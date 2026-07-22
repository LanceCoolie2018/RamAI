param(
    [Parameter(Mandatory = $true)]
    [string]$VcxprojPath
)

$content = Get-Content -Path $VcxprojPath -Raw
$replacement = '$(MSBuildProjectDirectory)\..\..\RamBedAI.uproject'
$content = $content -replace '\$\(SolutionDir\)RamBedAI\.uproject', $replacement
Set-Content -Path $VcxprojPath -Value $content -NoNewline