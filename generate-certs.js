const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const certsDir = path.join(__dirname, 'certs');

// Create certs directory if it doesn't exist
if (!fs.existsSync(certsDir)) {
    fs.mkdirSync(certsDir, { recursive: true });
}

const keyPath = path.join(certsDir, 'server.key');
const certPath = path.join(certsDir, 'server.cert');

// Check if certs already exist
if (fs.existsSync(keyPath) && fs.existsSync(certPath)) {
    console.log('SSL certificates already exist in ./certs/');
    console.log('Delete them and run this script again to regenerate.');
    process.exit(0);
}

console.log('Generating self-signed SSL certificates...');

try {
    // Generate self-signed certificate
    execSync(`openssl req -x509 -newkey rsa:4096 -keyout ${keyPath} -out ${certPath} -days 365 -nodes -subj "/C=US/ST=State/L=City/O=Organization/CN=localhost"`, {
        stdio: 'inherit'
    });
    
    console.log('\nSSL certificates generated successfully!');
    console.log(`  Key:  ${keyPath}`);
    console.log(`  Cert: ${certPath}`);
    console.log('\nNote: These are self-signed certificates for development.');
    console.log('For production, use certificates from a trusted CA (e.g., Let\'s Encrypt).');
} catch (error) {
    console.error('Failed to generate certificates:', error.message);
    console.error('\nMake sure OpenSSL is installed on your system.');
    process.exit(1);
}
