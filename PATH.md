# 🕹️ HTTPQuest

Build a full HTTP/1.1 server in C from raw POSIX sockets, using only man pages and RFCs.

**Rules**
- Only read the level you're on. No peeking ahead.
- A level is cleared only when its check passes exactly.
- Tick the boxes as you go, and write down what you learned. Future-you (and your blog post) will thank you.

Get the RFCs from https://www.rfc-editor.org (the plain-text versions are easiest to read).

---

## 📊 Progress

| World | Levels | Status |
|---|---|---|
| 1: The Handshake | 0–5 | ⬜ |
| 2: The Filesystem | 6–8 | ⬜ |
| 3: The Crowd | 9–12 | ⬜ |
| 🏆 Final Boss | 13 | ⬜ |

---

# 🌍 World 1: The Handshake

## Level 0 → 1: Plug the leaks

**Goal:** Your server must survive rude clients and fast restarts.

**Docs:** `recv(2)`, `setsockopt(2)`, `socket(7)`, `signal(7)`

**Check:**
- [ ] Run `nc localhost 3500` and press Ctrl-C without typing anything. The server stays up.
- [ ] Kill the server and restart it immediately. You get no "Address already in use."
- [ ] Run `curl localhost:3500 & kill %1` a few times. The server stays up.

**Notes:**
Passed all test without changing code.

---

## Level 1 → 2: Speak HTTP

**Goal:** Send a real HTTP/1.1 response with a status line, `Content-Type`, `Content-Length`, and CRLF line endings.

**Docs:** RFC 9112 §4, RFC 9110 §8.6

**Check:**
- [ ] `curl -v localhost:3500/` shows `HTTP/1.1 200 OK` and no warnings.

**Notes:**

---

## Level 2 → 3: Understand the request

**Goal:** Parse the request line.
- `GET /hello` returns `hello`
- An unknown path returns `404`
- A method other than GET returns `405` with an `Allow` header

**Docs:** RFC 9112 §3, RFC 9110 §15.5.5–15.5.6

**Check:**
- [ ] `curl -i localhost:3500/hello` returns `200` and `hello`
- [ ] `curl -i localhost:3500/nope` returns `404`
- [ ] `curl -i -X DELETE localhost:3500/hello` returns `405` with `Allow: GET`

**Notes:**

---

## Level 3 → 4: The stream is not a message

**Goal:** Read until the end of the headers, however the bytes arrive. Parse headers, and have `/ua` return the client's `User-Agent`.

**Docs:** RFC 9112 §2.1, §5

**Check:** This request arrives in two pieces, and it must still get a correct response:

```sh
(printf 'GET /ua HTTP/1.1\r\nHost: x\r\n'; sleep 1; printf 'User-Agent: slowpoke\r\n\r\n') | nc localhost 3500
```

- [ ] The response body is `slowpoke`

**Notes:**

---

## Level 4 → 5: Bodies

**Goal:** `POST /echo` returns exactly the body it received, using `Content-Length`.

**Docs:** RFC 9112 §6, `send(2)` (partial writes)

**Check:** The body is far bigger than your buffer:

```sh
head -c 300000 /dev/urandom | base64 > big.txt
curl -s --data-binary @big.txt localhost:3500/echo | cmp - big.txt && echo PASS
```

- [ ] Prints `PASS`

**Notes:**

---

## Level 5 → 6: Hostile input

**Goal:** Garbage gets `400`, oversized headers get `431`, and nothing crashes.

**Docs:** RFC 9112 §2.2, RFC 6585 §5

**Check:**

```sh
printf 'lol what\r\n\r\n' | nc localhost 3500
(printf 'GET / HTTP/1.1\r\nX: '; head -c 50000 /dev/zero | tr '\0' 'a'; printf '\r\n\r\n') | nc localhost 3500
```

- [ ] First command returns `400`
- [ ] Second command returns `431`
- [ ] Server still answers `curl localhost:3500/hello` afterwards

**Notes:**

---

> 🔓 **WORLD 1 CLEAR:** Open `http://localhost:3500/hello` in a real browser. It works because you now speak the protocol.

---

# 🌍 World 2: The Filesystem

## Level 6 → 7: Serve a directory

**Goal:** Serve files from `./www`, with `Content-Type` set by extension, and make binary files work.

**Docs:** `open(2)`, `fstat(2)`, `read(2)`

**Check:** Put an `index.html` and a `.png` in `www/`, then:

```sh
curl -s localhost:3500/cat.png | cmp - www/cat.png && echo PASS
curl -I localhost:3500/index.html
```

- [ ] First command prints `PASS`
- [ ] Second command shows `Content-Type: text/html`
- [ ] A missing file returns `404`

**Notes:**

---

## Level 7 → 8: The traversal trap

**Goal:** Nobody escapes `./www`.

**Docs:** `realpath(3)`, RFC 3986 §2.1 (percent-encoding)

**Check:** Both must return `403` or `404`, never the file:

```sh
curl --path-as-is localhost:3500/../../etc/passwd
curl --path-as-is localhost:3500/%2e%2e/%2e%2e/etc/passwd
```

- [ ] Plain `../` is blocked
- [ ] Percent-encoded `%2e%2e` is blocked

**Notes:**

---

## Level 8 → 9: HEAD

**Goal:** HEAD returns the same headers as GET, including the correct `Content-Length`, but no body.

**Docs:** RFC 9110 §9.3.2

**Check:**
- [ ] `curl -I localhost:3500/cat.png` shows the real file size
- [ ] No body bytes are sent

**Notes:**

---

> 🔓 **WORLD 2 CLEAR:** Bind to your LAN IP and open your own HTML page on your phone.

---

# 🌍 World 3: The Crowd

## Level 9 → 10: The frozen server

**Goal:** One idle client must not block everyone else, and the server leaves no zombies.

**Docs:** `fork(2)`, `waitpid(2)`, SIGCHLD in `signal(7)`

**Check:**
- [ ] With `nc localhost 3500` idle in one terminal, `curl localhost:3500/hello` still works in another
- [ ] After 100 requests, `ps aux | grep defunct` shows nothing from your server

**Notes:**

---

## Level 10 → 11: Keep-alive

**Goal:** Handle multiple requests on one connection, including two requests that arrive in the same read.

**Docs:** RFC 9112 §9.3

**Check:**

```sh
curl -v localhost:3500/hello localhost:3500/hello
printf 'GET /hello HTTP/1.1\r\nHost: x\r\n\r\nGET /hello HTTP/1.1\r\nHost: x\r\n\r\n' | nc localhost 3500
```

- [ ] curl shows "Re-using existing connection"
- [ ] nc receives two full responses

**Notes:**

---

## Level 11 → 12: Timeouts

**Goal:** Idle connections are closed after N seconds.

**Docs:** `setsockopt(2)` (SO_RCVTIMEO) or `poll(2)`

**Check:**
- [ ] `nc localhost 3500` with no typing gets disconnected on its own

**Notes:**

---

## Level 12 → 13: The event loop

**Goal:** Build a single-threaded, non-blocking server with epoll that handles 1000 concurrent connections.

**Docs:** `fcntl(2)`, `epoll(7)`, `epoll_ctl(2)`, `epoll_wait(2)`

**Check:**

```sh
ab -k -c 1000 -n 100000 http://localhost:3500/hello
```

- [ ] Zero failed requests
- [ ] Clean run with the server built using `-fsanitize=address`

**Notes:**

---

> 🔓 **WORLD 3 CLEAR:** Benchmark your fork version against your epoll version and record the numbers below.

| Version | Requests/sec | Failed | Notes |
|---|---|---|---|
| fork | | | |
| threads (optional) | | | |
| epoll | | | |

---

# 🏆 Final Boss: Go live

**Goal:**
- [ ] Deploy your server to your VPS
- [ ] Put Caddy in front of it (Caddy handles TLS and reverse-proxies to your server)
- [ ] Serve a real subdomain of dvippatel.in
- [ ] `curl -I https://<your-subdomain>` shows `Server: dvip-httpd/1.0`
- [ ] Write the blog post about building it from man pages alone

**Reward:** A live website on the internet, running on an HTTP server you wrote from scratch.

---

# 📎 Appendix: Automated grader prompt (optional)

Paste this into Claude Code to build a Bandit-style grader that gives you real passwords for each level.

```
Build a CLI grader called "httpquest" for a learner writing an HTTP/1.1 server in C
from raw POSIX sockets. It is a Bandit-style wargame (like overthewire.org/wargames/bandit).

HARD RULES:
- Do NOT read, modify, or write any code in the learner's server directory. You only
  build the grader. The learner is deliberately learning from man pages and RFCs only.
- Failure messages say WHAT failed (expected vs. actual, raw bytes if useful) and point
  to the relevant man page or RFC section. They must never explain HOW to fix it or
  show server code.
- Python 3 standard library only. Single file. Tests talk to the server over raw sockets
  (not http.client), so the grader controls byte-level timing and malformed input.

MECHANICS:
- Usage: `httpquest status`, `httpquest play <level> <password>`, `httpquest brief <level>`.
- `brief` prints the level's goal and docs, Bandit-style. Levels stay hidden until unlocked.
- Passing level N prints the password for level N+1. Passwords are derived with HMAC from
  a random secret generated on first run and stored in ~/.httpquest/ along with progress.
- Target host and port are configurable (default localhost:3500).
- Each world completion prints an ASCII-art unlock banner. The final level prints a
  big victory screen.

LEVELS (implement a test for each; add edge cases that are hard to fake):
0 Survives: client connect+close with no data, client disconnect before response
  (SIGPIPE), immediate restart (grader asks learner to restart, then reconnects).
1 Valid HTTP/1.1 status line, Content-Type, correct Content-Length, CRLF endings.
2 Request-line parsing: GET /hello -> "hello"; unknown path -> 404; DELETE -> 405 with Allow.
3 Headers split across multiple TCP writes with delays (byte-by-byte variant too);
  /ua echoes the User-Agent.
4 POST /echo with bodies up to 1 MB, including binary; checks exact bytes returned.
5 Malformed request line -> 400; 50 KB header -> 431; server still alive afterwards.
6 Static files from ./www with correct MIME types; binary file byte-exact (grader creates
  the test files and tells the learner where to put them).
7 Path traversal: ../, %2e%2e, mixed encodings, absolute paths -> never leaks a file
  outside www.
8 HEAD matches GET headers exactly, with an empty body.
9 An idle connection does not block a second client.
10 Keep-alive: sequential requests on one socket, plus two pipelined requests in one write.
11 Idle connection closed by the server within a timeout the grader announces.
12 1000 concurrent connections (use threads or selectors in the grader), zero failures,
  latency summary printed.
13 Final: learner passes a public https URL; the grader verifies TLS, a 200 response, and a
  custom Server header.

Include a README with install and usage instructions. Test the grader against a tiny
throwaway reference server you write in /tmp, then delete that server so it can't be
used as a crib.
```
