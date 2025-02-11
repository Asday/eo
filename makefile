-include build/config.make
-include build/config-server.make

.PHONY: all
all: cluster launcher login client

cluster: src/servers/cluster/main.cpp
cluster: build/config.make
cluster: build/config-server.make
	$(CXX) \
		$(CXXFLAGS-server) $(CXXFLAGS) \
		$(LDFLAGS-server) $(LDFLAGS) \
		src/servers/cluster/main.cpp \
		-o $@

launcher: src/servers/launcher/main.cpp
launcher: build/config.make
launcher: build/config-server.make
	$(CXX) \
		$(CXXFLAGS-server) $(CXXFLAGS) \
		$(LDFLAGS-server) $(LDFLAGS) \
		src/servers/launcher/main.cpp \
		-o $@

login: src/servers/login/main.cpp
login: build/config.make
login: build/config-server.make
	$(CXX) \
		$(CXXFLAGS-server) $(CXXFLAGS) \
		$(LDFLAGS-server) $(LDFLAGS) \
		src/servers/login/main.cpp \
		-o $@

client: src/client/main.cpp build/config.make
	$(CXX) $(CXXFLAGS) $(LDFLAGS) src/client/main.cpp -o $@

vm/base.qcow2:
	wget \
		https://geo.mirror.pkgbuild.com/images/latest/Arch-Linux-x86_64-basic.qcow2 \
		-O vm/base.qcow2

.PHONY: eo-vm-bridge
eo-vm-bridge:
	ip link show $@ \
		|| ( \
			sudo ip link add name $@ type bridge \
			&& sudo ip link set dev $@ up \
		)

.PHONY: eo-vm-network-iptables
eo-vm-network-iptables:
	sudo iptables -I FORWARD -m physdev --physdev-is-bridged -j ACCEPT

.PHONY: eo-vm-network-qemu-conf
eo-vm-network-qemu-conf:
	grep eo-vm-bridge /etc/qemu/bridge.conf || \
		echo "allow eo-vm-bridge" | sudo tee --append /etc/qemu/bridge.conf

.PHONY: eo-vm-network
eo-vm-network: eo-vm-bridge eo-vm-network-qemu-conf eo-vm-network-iptables

.PHONY: run-vm
run-vm: vm/base.qcow2 eo-vm-network
	qemu-system-x86_64 \
		-drive file=vm/base.qcow2,if=virtio \
		-nic user,model=virtio-net-pci,id=public0,smb=vm/shared \
		-nic bridge,model=virtio-net-pci,id=private0,br=eo-vm-bridge \
		-cpu host -machine q35 -accel kvm -device intel-iommu \
		-m 2048M

.PHONY: clean-bin
clean-bin:
	rm -f cluster launcher login client

.PHONY: clean-vm-network-bridge
clean-vm-network-bridge:
	sudo ip link delete eo-vm-bridge type bridge || echo "no bridge"

.PHONY: clean-vm-network-qemu-conf
clean-vm-network-qemu-conf:
	sudo sed -i '/eo-vm-bridge/d' /etc/qemu/bridge.conf

.PHONY: clean-vm-network-iptables
clean-vm-network-iptables:
	echo "todo"

.PHONY: clean-vm-network
clean-vm-network: clean-vm-network-bridge clean-vm-network-qemu-conf

.PHONY: clean-vm-base
clean-vm-base:
	rm -f vm/base.qcow2

.PHONY: clean
clean: clean-bin clean-vm-network

.PHONY: nuke
nuke: clean clean-vm-base
