# Validation Gates

1. Verify the history backup and working-state snapshot exist and are readable.
2. Enumerate all candidate branches, tags, and files.
3. Run a redacted secret scan across the full candidate history.
4. Confirm excluded paths and deployment-specific identifiers have zero matches.
5. Validate portfolio-document links.
6. Run Functions installation, compilation, and tests.
7. Run Android unit tests and debug assembly with placeholder configuration.
8. Push to a private verification repository and require successful CI.
9. Confirm the original repository remains private before and after replacement.
