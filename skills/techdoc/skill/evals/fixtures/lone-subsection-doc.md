# Cache Configuration

The cache stores computed results to avoid recomputation.

## Settings

### TTL

The `ttl` setting controls how long an entry lives before it expires. It is
expressed in seconds. When an entry is older than the TTL, the next read
recomputes it and stores the fresh value.

#### Edge cases

When the TTL is zero, caching is effectively disabled because every entry is
immediately considered expired.
