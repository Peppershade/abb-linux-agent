# Secure Boot Support

> **Experimental — No support provided. These instructions are untested and
> may not work.** They are provided as-is based on the upstream Axcient guide,
> without any verification against this project. Use at your own risk.
> If it works or fails for you, please
> [open an issue](https://github.com/Peppershade/abb-linux-agent/issues)
> and report your distribution and kernel version — this helps others.

The `synosnap` DKMS module can be made to work with UEFI Secure Boot by
enrolling a Machine Owner Key (MOK) before installation. The approach is based
on [Axcient's guide for the xCloud Agent](https://axcient.helpjuice.com/x360recover-faqs-specific-to-linux/install-xcloud-agent-for-linux-with-uefi-secure-boot-x360recover),
which uses the same underlying `elastio-snap` kernel module.

---

## How it works

The `synosnap` package ships a `dkms_pre_install` script that:
- Detects whether Secure Boot is active via `mokutil`
- Generates an MOK key pair in `/opt/synosnap/` (if not already present)
- Signs the compiled module using the kernel's own `sign-file` utility

On **Ubuntu**, the script defers to Ubuntu's native DKMS signing mechanism
instead, which handles signing automatically — provided an MOK key has been
enrolled beforehand.

Either way, the key enrollment itself is a one-time manual step that must be
completed **before** running the installer.

---

## Prerequisites

```bash
sudo apt install -y mokutil openssl dkms
```

---

## Step 1 — Generate a signing key

```bash
sudo mkdir -p /opt/synosnap
sudo openssl req -new -x509 -batch -sha256 -nodes -days 36500 \
  -subj "/CN=$(hostname -s) Secure Boot Module Signature key/O=$(hostname -s)" \
  -keyout /opt/synosnap/MOK.priv \
  -out /opt/synosnap/MOK.pem
sudo openssl x509 -in /opt/synosnap/MOK.pem -outform DER \
  -out /opt/synosnap/MOK.der
sudo chmod 700 /opt/synosnap/MOK.priv
```

---

## Step 2 — Enroll the key

```bash
sudo mokutil --import /opt/synosnap/MOK.der
```

You will be prompted to set a one-time password. Remember it — you will need
it on the next boot.

---

## Step 3 — Reboot and confirm enrollment

Reboot your machine. During boot, the MOK manager will appear (you have ~10
seconds to press a key). Choose:

1. **Enroll MOK**
2. **Continue**
3. Enter the password you set in Step 2
4. **Reboot**

---

## Step 4 — Configure DKMS to use the key (Ubuntu only)

Ubuntu's DKMS signs modules automatically but needs to know which key to use.
Add the following to `/etc/dkms/framework.conf` (create it if it doesn't
exist):

```bash
mok_signing_key="/opt/synosnap/MOK.priv"
mok_certificate="/opt/synosnap/MOK.pem"
sign_tool="/etc/dkms/sign_helper.sh"
```

Then create `/etc/dkms/sign_helper.sh`:

```bash
#!/bin/sh
/usr/src/linux-headers-$(uname -r)/scripts/sign-file sha256 \
  /opt/synosnap/MOK.priv \
  /opt/synosnap/MOK.pem "$@"
```

```bash
sudo chmod +x /etc/dkms/sign_helper.sh
```

---

## Step 5 — Install the agent

Run the installer as normal:

```bash
sudo bash install.run
```

The `synosnap` module will be built, signed, and loaded automatically.

---

## Verification

After installation, confirm the module is loaded:

```bash
lsmod | grep synosnap
```

If the module fails to load, check the kernel log:

```bash
sudo dmesg | grep -i "synosnap\|module\|secure\|sign"
```

---

## Credits

This approach is adapted from the
[Axcient xCloud Agent Secure Boot guide](https://axcient.helpjuice.com/x360recover-faqs-specific-to-linux/install-xcloud-agent-for-linux-with-uefi-secure-boot-x360recover).
The `synosnap` kernel module is based on Axcient's
[elastio-snap](https://github.com/Axcient/elastio-snap), so the same signing
approach applies.
