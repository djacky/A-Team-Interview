// Step 1) npm i mongodb (installs mongodb driver)
// Step 2) run the script below with your desired new password
// Step 3) If successful, test the new password, change it in the .env of your EC2
// Step 4) replace dbPassword below with your new password
// Step 5) npm uninstall mongodb

const { MongoClient } = require('mongodb');

// Replace these variables with your actual values
const dbHost = "18.185.127.22"; // IPV4 IP of EC2
const dbPort = 27017;
const dbName = "kode_zero";
const dbUser = "admin";
const dbPassword = "yccNc4eEIipscjcoHvtZPXNWeUUkMuYkPbrqDLSGYvs8vldU4g1eLHoz31p3tcrd8kraDMhJxxZVg1maiw";
const dbAuthSource = "admin";

const newUserPassword = 'newPassword';  // Replace with the new password you want to set

// Construct the MongoDB URI
const uri = `mongodb://${dbUser}:${dbPassword}@${dbHost}:${dbPort}/${dbName}?authSource=${dbAuthSource}`;

async function changePassword() {
    const client = new MongoClient(uri, { useNewUrlParser: true, useUnifiedTopology: true });

    try {
        // Connect to the MongoDB server
        await client.connect();

        // Select the admin database for user management operations
        const adminDb = client.db('admin');

        // Update the user password
        await adminDb.command({
            updateUser: dbUser,
            pwd: newUserPassword,
        });

        console.log('Password updated successfully');
    } catch (error) {
        console.error('Error updating password:', error);
    } finally {
        // Close the connection
        await client.close();
    }
}

changePassword();