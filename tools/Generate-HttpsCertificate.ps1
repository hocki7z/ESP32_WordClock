# Run by PlatformIO on the first HTTPS build. Never prints the private key.
# Requires PowerShell 7; no certificate is installed into a system trust store.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [ValidatePattern('^[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?$')]
    [string]$Hostname = 'wordclock',
    [string[]]$IpAddress = @()
)

$ErrorActionPreference = 'Stop'
$certificatePath = Join-Path $OutputDirectory 'server.crt'
$keyPath = Join-Path $OutputDirectory 'server.key'
if ((Test-Path $certificatePath) -or (Test-Path $keyPath)) {
    throw 'TLS identity already exists. Back it up and remove BOTH files explicitly before regenerating.'
}
[void][System.IO.Directory]::CreateDirectory($OutputDirectory)
$rsa = [System.Security.Cryptography.RSA]::Create(2048)
$certificate = $null
try {
    $request = [System.Security.Cryptography.X509Certificates.CertificateRequest]::new(
        "CN=$Hostname.local", $rsa,
        [System.Security.Cryptography.HashAlgorithmName]::SHA256,
        [System.Security.Cryptography.RSASignaturePadding]::Pkcs1)
    $san = [System.Security.Cryptography.X509Certificates.SubjectAlternativeNameBuilder]::new()
    $san.AddDnsName("$Hostname.local")
    $san.AddDnsName($Hostname)
    foreach ($address in $IpAddress) { $san.AddIpAddress([System.Net.IPAddress]::Parse($address)) }
    $request.CertificateExtensions.Add($san.Build())
    $request.CertificateExtensions.Add(
        [System.Security.Cryptography.X509Certificates.X509BasicConstraintsExtension]::new($false, $false, 0, $true))
    $usages = [System.Security.Cryptography.X509Certificates.X509KeyUsageFlags]::DigitalSignature -bor
        [System.Security.Cryptography.X509Certificates.X509KeyUsageFlags]::KeyEncipherment
    $request.CertificateExtensions.Add(
        [System.Security.Cryptography.X509Certificates.X509KeyUsageExtension]::new($usages, $true))
    $oids = [System.Security.Cryptography.OidCollection]::new()
    [void]$oids.Add([System.Security.Cryptography.Oid]::new('1.3.6.1.5.5.7.3.1')) # TLS server authentication
    $request.CertificateExtensions.Add(
        [System.Security.Cryptography.X509Certificates.X509EnhancedKeyUsageExtension]::new($oids, $false))
    $certificate = $request.CreateSelfSigned([DateTimeOffset]::UtcNow.AddDays(-1), [DateTimeOffset]::UtcNow.AddDays(730))
    $options = [System.Base64FormattingOptions]::InsertLineBreaks
    $certBytes = $certificate.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Cert)
    $keyBytes = $rsa.ExportPkcs8PrivateKey()
    $certPem = "-----BEGIN CERTIFICATE-----`n" + [Convert]::ToBase64String($certBytes, $options).Replace("`r`n", "`n") + "`n-----END CERTIFICATE-----`n"
    $keyPem = "-----BEGIN PRIVATE KEY-----`n" + [Convert]::ToBase64String($keyBytes, $options).Replace("`r`n", "`n") + "`n-----END PRIVATE KEY-----`n"
    [System.IO.File]::WriteAllText($keyPath, $keyPem, [System.Text.Encoding]::ASCII)
    [System.IO.File]::WriteAllText($certificatePath, $certPem, [System.Text.Encoding]::ASCII)
    Write-Host "Created unique self-signed HTTPS identity for $Hostname.local (valid 730 days)."
    Write-Host 'The local .https directory and firmware contain a private key; do not share them.'
}
finally {
    if ($null -ne $certificate) { $certificate.Dispose() }
    $rsa.Dispose()
}