import java.nio.file.*;
import java.security.*;
import java.security.spec.*;
import java.security.cert.*;
import java.io.*;
import java.util.*;
import org.bouncycastle.crypto.generators.Argon2BytesGenerator;
import org.bouncycastle.crypto.params.Argon2Parameters;

class reference_security
{
    static byte[] derive(byte[] password, byte[] salt)
    {
        Argon2Parameters parameters = new Argon2Parameters.Builder(Argon2Parameters.ARGON2_id)
            .withVersion(Argon2Parameters.ARGON2_VERSION_13).withMemoryAsKB(19456)
            .withIterations(2).withParallelism(1).withSalt(salt).build();
        Argon2BytesGenerator generator = new Argon2BytesGenerator();
        generator.init(parameters);
        byte[] result = new byte[32];
        generator.generateBytes(password, result);
        return result;
    }

    static void run() throws Exception
    {
        byte[] data = Files.readAllBytes(Path.of("message.bin"));
        byte[] other = data.clone();
        long total = 0;
        long start = System.nanoTime();
        for (int i = 0; i < 20000; i++)
        {
            total += MessageDigest.isEqual(data, other) ? 1 : 0;
        }
        reference_java.report("secret_equal", start, total);
        byte[] password = Files.readAllBytes(Path.of("password.bin"));
        SecureRandom random = new SecureRandom();
        start = System.nanoTime();
        total = 0;
        for (int i = 0; i < 3; i++)
        {
            byte[] salt = new byte[16];
            random.nextBytes(salt);
            byte[] hash = derive(password, salt);
            String encoded = "$argon2id$v=19$m=19456,t=2,p=1$" + Base64.getEncoder().withoutPadding().encodeToString(salt)
                + "$" + Base64.getEncoder().withoutPadding().encodeToString(hash);
            String[] parts = encoded.split("\\$");
            byte[] again = derive(password, Base64.getDecoder().decode(parts[4]));
            total += MessageDigest.isEqual(again, Base64.getDecoder().decode(parts[5])) ? 1 : 0;
        }
        reference_java.report("argon2_hash_verify", start, total);
        KeyFactory factory = KeyFactory.getInstance("Ed25519");
        PrivateKey key = factory.generatePrivate(new PKCS8EncodedKeySpec(Files.readAllBytes(Path.of("ed_private.der"))));
        PublicKey pub = factory.generatePublic(new X509EncodedKeySpec(Files.readAllBytes(Path.of("ed_public.der"))));
        Signature signer = Signature.getInstance("Ed25519");
        signer.initSign(key);
        byte[] signature = null;
        start = System.nanoTime();
        total = 0;
        for (int i = 0; i < 500; i++)
        {
            signer.update(data);
            signature = signer.sign();
            total += signature.length;
        }
        reference_java.report("ed25519_sign", start, total);
        Signature verifier = Signature.getInstance("Ed25519");
        verifier.initVerify(pub);
        start = System.nanoTime();
        total = 0;
        for (int i = 0; i < 500; i++)
        {
            verifier.update(data);
            total += verifier.verify(signature) ? 1 : 0;
        }
        reference_java.report("ed25519_verify", start, total);
        byte[] cert = Files.readAllBytes(Path.of("server.der"));
        CertificateFactory certificates = CertificateFactory.getInstance("X.509");
        start = System.nanoTime();
        total = 0;
        for (int i = 0; i < 500; i++)
        {
            total += certificates.generateCertificate(new ByteArrayInputStream(cert)).getEncoded().length;
        }
        reference_java.report("x509_parse_der", start, total);
    }
}
