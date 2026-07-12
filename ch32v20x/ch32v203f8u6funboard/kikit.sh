kikit panelize \
	--layout 'grid; rows: 2; cols: 2; hspace:2mm' \
	--tabs 'fixed; vcount: 1; hcount: 0; vwidth: 30mm' \
	--cuts 'vcuts; layer: User.1' \
	--framing 'railstb; width: 5mm; fillet: 2mm' \
	--tooling '3hole; hoffset: 3mm; voffset: 2.5mm; size: 2mm' \
	--fiducials '3fid; hoffset: 7mm; voffset: 2.5mm; coppersize: 3mm; opening: 1.5mm' \
	--post 'millradius: 0.6mm' \
	--text 'simple; text: Panel made on: {date} .; anchor: mt; voffset: 2.5mm; hjustify: center; vjustify:center' \
	./ch32v203f8u6funboard.kicad_pcb ./ch32v203f8u6funboard_panel.kicad_pcb
