all : subst index.html

#For tools (Works in Ubuntu 20.04 (Including WSL), Mint)
# sudo apt-get install clang-10 lld-10 binaryen
# sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-10 20
# sudo update-alternatives --install /usr/bin/wasm-ld wasm-ld /usr/bin/wasm-ld-10 20

#node-uglify lld clang-10 lld-10 binaryen

#Path to rawdraw
CFLAGS:=-I.. -DEMU -I../ch32v003fun/extralibs -Iweblibc_headers -DWASM -DASYNCIFY=1

CLANG?=clang
WASMOPT?=wasm-opt
TERSER?=terser
TERSERFLAGS?=--module --ecma 2017 -d RAWDRAW_USE_LOOP_FUNCTION=false -d RAWDRAW_NEED_BLITTER=true

CFLAGS+=-DWASM -nostdlib --target=wasm32 \
		-flto -Oz \
		-Wl,--lto-O3 \
		-Wl,--no-entry \
		-Wl,--allow-undefined \
		-mbulk-memory
#		-Wl,--import-memory

WOFLAGS:=--asyncify #  --pass-arg=asyncify-ignore-indirect
#--pass-arg=asyncify-imports@bynsyncify.* 

opt.js : template.js main.wasm
	cat main.wasm | gzip -9 | dd bs=1 skip=10 | head -c -8 | base64 | sed -e "$$ ! {/./s/$$/ \\\\/}" > blob_b64;
	./subst template.js -s -f BLOB blob_b64 -o mid.js
	#Comment the below line out if you don't want to uglify the javascript.
	$(TERSER) $(TERSERFLAGS) mid.js -o opt.js
	#rm mid.js blob_b64

index.html : template.ht opt.js
	./subst template.ht -s -f JAVASCRIPT_DATA opt.js -o $@

subst : subst.c
	cc -o $@ $^

main.wasm: ../sao.c
	$(CLANG) $(CFLAGS) $^ -o $@
	$(WASMOPT) $(WOFLAGS) main.wasm -o main.wasm
	#wasm-objdump -d main.wasm > main.disassembly.txt

clean:
	rm -rf main.wasm opt.js index.html blob_b64

