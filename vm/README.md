# ft_ping dev VM

Debian 12 aarch64 QEMU VM (HVF-accelerated) for building/testing `ft_ping` on
real Linux.

## Use

```sh
brew install qemu
make vm-setup      # once: downloads image, builds disk + cloud-init seed
make vm            # boot (headless, backgrounded; returns when ready)
make vm-ssh        # log in
make vm-stop       # shut down
```

The VM runs in the background with no console — access is SSH only
(`ssh -p 2222 -i vm/id_ed25519 debian@localhost`, user/password `debian`). Boot
output goes to `vm/console.log`. First `make vm` takes ~2 min (provisioning plus
one automatic reboot); later boots are ~10s.

```sh
cd /mnt/ft_ping && make re && sudo ./ft_ping 8.8.8.8
```

The project is live-shared at `/mnt/ft_ping` via 9p. Always `make re` inside the
VM — macOS `.o` files are Mach-O and won't link there.

Reset: `make vm-clean && make vm-setup`. Tuning: `RAM=8G CPUS=8 SSH_PORT=2223 vm/run.sh`.

## Gotchas

- **Run as `sudo`.** Raw ICMP sockets need `CAP_NET_RAW`. `setcap` fails on the
  9p share (macOS backend lacks the `security.capability` xattr).
- **TTL doesn't work on default networking.** Slirp proxies ICMP via a host
  socket: echo request/reply is fine, but TTL is faked (`ttl=255`) and ICMP
  time-exceeded (type 11) never arrives. For `-t`/TTL work use
  `sudo NET=vmnet vm/run.sh` — real NAT interface, but needs root, no
  `localhost:2222` forward (SSH to the guest IP from `ip a`), and guest-created
  files land root-owned on the Mac.

Two cloud-image quirks handled in [cloud-init/user-data](cloud-init/user-data):
the cloud kernel has no 9p modules (replaced with `linux-image-arm64`), and the
guest user is created with your macOS UID (9p2000.L can't remap ownership).
