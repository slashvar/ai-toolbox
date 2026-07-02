# How Retry Behavior Is Handled In The Client Library

Audience: backend engineers
Tech level: intermediate

## Overview

In this section we will describe the retry behavior. It should be noted that
the client is configured in order to retry requests. When a request is not
successful, the request will be retried by the client. There are several
options that can be set by the user, and these options are rather important for
tuning the behavior of the retry logic in production environments.

### Options

The timeout option controls the timeout. The retries option is used for
configuring the number of retries. The backoff option is also configurable and
it controls the delay between attempts.
