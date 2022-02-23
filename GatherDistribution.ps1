$libs = "Neo Steelgear Graphics Scene 64.lib", "Neo Steelgear Graphics Scene 64D.lib", "Neo Steelgear Graphics Scene 32.lib", "Neo Steelgear Graphics Scene 32D.lib"

Get-ChildItem -Path "Distribute" -Include *.* -File -Recurse | foreach { $_.Delete()}

foreach ($file in $libs)
{
	

	$path = Get-ChildItem -Path "" -Filter $file -Recurse -ErrorAction SilentlyContinue -Force
	if ($path)
	{
		Copy-Item $path.FullName -Destination "Distribute\\Libraries\\"
	}
	else
	{
		Write-Host "Could not find file:" $file
	}
}

Get-ChildItem -Path "Neo Steelgear Graphics Scene" -Include *.h* -File -Recurse | foreach { Copy-Item $_.FullName -Destination "Distribute\\Headers\\"}