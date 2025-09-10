param([string]$RegistrationId)

Write-Output "#define democonfigREGISTRATION_ID    `"$RegistrationId`"`n"

# Create ECDSA key using NIST P-256
$curve = [System.Security.Cryptography.ECCurve]::CreateFromFriendlyName("nistP256")
$key = [System.Security.Cryptography.ECDsa]::Create($curve)


# Export private key in PKCS#8 format
$keyBlob = $key.Key.Export([System.Security.Cryptography.CngKeyBlobFormat]::Pkcs8PrivateBlob)
$keyBase64 = [Convert]::ToBase64String($keyBlob, [System.Base64FormattingOptions]::InsertLineBreaks)
$keyBase64 = $keyBase64 -replace "`r`n", "`\r`\n`"    `\`n`""

$keyPem = "#define democonfigCLIENT_PRIVATE_KEY_PEM    `\`n`"-----BEGIN PRIVATE KEY-----`\r`\n`"    `\`n`"$keyBase64`\r`\n`"    `\`n`"-----END PRIVATE KEY-----`\r`\n`""

Write-Output "$keyPem`n"

# Create CSR
$csr = New-Object System.Security.Cryptography.X509Certificates.CertificateRequest("CN=$RegistrationId", $key, [System.Security.Cryptography.HashAlgorithmName]::SHA256)

# Export CSR
$csrBytes = $csr.CreateSigningRequest()
$csrBase64 = [Convert]::ToBase64String($csrBytes)

# Print CSR macro
Write-Output "#define democonfigCERTIFICATE_SIGNING_REQUEST `"$csrBase64`""


