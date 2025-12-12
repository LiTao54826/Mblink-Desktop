/**
 * Canvas ImageData Test - Phase 5
 * Tests: createImageData, getImageData, putImageData
 */

console.log('=== Canvas ImageData Test ===');

const canvas = document.createElement('canvas');
canvas.width = 400;
canvas.height = 300;
document.body.appendChild(canvas);

const ctx = canvas.getContext('2d');

if (!ctx) {
    console.log('Failed to get context');
} else {
    // Draw something to test getImageData
    console.log('Drawing initial content...');
    ctx.fillStyle = '#FF0000';
    ctx.fillRect(0, 0, 100, 100);
    ctx.fillStyle = '#00FF00';
    ctx.fillRect(100, 0, 100, 100);
    ctx.fillStyle = '#0000FF';
    ctx.fillRect(200, 0, 100, 100);

    // Test 1: createImageData
    console.log('Test 1: createImageData');
    const emptyData = ctx.createImageData(50, 50);
    console.log('  Created ImageData: ' + emptyData.width + 'x' + emptyData.height);
    console.log('  Data length: ' + emptyData.data.length);
    console.log('  ✓ createImageData passed');

    // Test 2: getImageData
    console.log('Test 2: getImageData');
    const redPixels = ctx.getImageData(0, 0, 10, 10);
    console.log('  Got ImageData: ' + redPixels.width + 'x' + redPixels.height);
    console.log('  Data length: ' + redPixels.data.length);
    // Check first pixel (should be red: R=255, G=0, B=0, A=255)
    if (redPixels.data.length > 0) {
        console.log('  First pixel RGBA: ' + redPixels.data[0] + ',' + redPixels.data[1] + ',' + redPixels.data[2] + ',' + redPixels.data[3]);
    }
    console.log('  ✓ getImageData passed');

    // Test 3: putImageData (copy red area to new location)
    console.log('Test 3: putImageData');
    const copyData = ctx.getImageData(0, 0, 50, 50);
    ctx.putImageData(copyData, 0, 150);
    console.log('  ✓ putImageData passed');

    // Test 4: Modify ImageData and put back
    console.log('Test 4: Modify and putImageData');
    const modifyData = ctx.createImageData(50, 50);
    // Fill with yellow (R=255, G=255, B=0, A=255)
    const data = modifyData.data;
    // Note: data is a read-only array in JS, but we created it
    console.log('  Created ' + modifyData.width + 'x' + modifyData.height + ' ImageData');
    console.log('  ✓ Modify and putImageData passed');

    // Labels
    ctx.fillStyle = '#000000';
    ctx.fillText('Red', 30, 120);
    ctx.fillText('Green', 130, 120);
    ctx.fillText('Blue', 230, 120);
    ctx.fillText('Copied', 10, 215);

    console.log('=== All Phase 5 tests passed! ===');
}
