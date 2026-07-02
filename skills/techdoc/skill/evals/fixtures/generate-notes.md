# Raw notes — deploy CLI

these are rough notes, turn into a doc

- command is `deploy push`
- takes --env (staging|prod), required
- --dry-run flag, prints plan, does nothing
- --timeout, default 300s, how long to wait for rollout
- on failure it auto-rolls back unless --no-rollback
- needs DEPLOY_TOKEN env var set
- exit code 0 ok, 1 failed, 2 bad args
