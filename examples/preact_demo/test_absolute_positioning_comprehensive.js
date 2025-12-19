/**
 * @file test_absolute_positioning_comprehensive.js
 * @brief Comprehensive absolute positioning demo covering all inset combinations
 * 
 * This demo tests all absolute positioning scenarios:
 * - bottom: 0 (Requirement 4.1)
 * - right: 0 (Requirement 4.2)
 * - top + bottom stretch (Requirement 4.3)
 * - left + right stretch (Requirement 4.4)
 * - All four insets (full stretch)
 * - Percentage insets
 * - Pixel value offsets (Requirement 4.5)
 * - Nested absolute positioning
 * - Viewport units with absolute positioning
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    // Container styles
    var containerStyle = {
        position: 'relative',
        width: '300px',
        height: '200px',
        backgroundColor: '#f5f5f5',
        border: '2px solid #333',
        margin: '10px',
        display: 'inline-block',
        verticalAlign: 'top'
    };

    var labelStyle = {
        fontSize: '11px',
        color: 'white',
        padding: '4px',
        textAlign: 'center'
    };

    var titleStyle = {
        position: 'absolute',
        top: '5px',
        left: '5px',
        fontSize: '12px',
        fontWeight: 'bold',
        color: '#333'
    };

    // Test container component
    function TestBox(props) {
        return h('div', { style: containerStyle },
            h('div', { style: titleStyle }, props.title),
            props.children
        );
    }

    // Positioned element component
    function PositionedElement(props) {
        var style = Object.assign({
            position: 'absolute',
            backgroundColor: props.color || 'rgba(52, 152, 219, 0.8)'
        }, props.style);
        
        return h('div', { style: style },
            h('span', { style: labelStyle }, props.label)
        );
    }

    function App() {
        return h('div', { 
            style: { 
                padding: '20px',
                backgroundColor: '#e0e0e0',
                minHeight: '100vh',
                fontFamily: 'Arial, sans-serif'
            } 
        },
            h('h1', { style: { margin: '0 0 20px 0', fontSize: '24px' } }, 
                'Absolute Positioning Comprehensive Test'),
            h('p', { style: { margin: '0 0 20px 0', color: '#666' } }, 
                'Testing all inset combinations per Requirements 4.1-4.5'),
            
            // Row 1: Basic single insets
            h('div', { style: { marginBottom: '20px' } },
                h('h2', { style: { fontSize: '16px', margin: '0 0 10px 0' } }, 
                    'Basic Single Insets'),
                
                // Test 1: bottom: 0 only (Req 4.1)
                h(TestBox, { title: '1. bottom: 0' },
                    h(PositionedElement, {
                        style: { bottom: '0', left: '50px', width: '80px', height: '50px' },
                        color: 'rgba(231, 76, 60, 0.8)',
                        label: 'bottom: 0'
                    })
                ),
                
                // Test 2: right: 0 only (Req 4.2)
                h(TestBox, { title: '2. right: 0' },
                    h(PositionedElement, {
                        style: { right: '0', top: '50px', width: '80px', height: '50px' },
                        color: 'rgba(46, 204, 113, 0.8)',
                        label: 'right: 0'
                    })
                ),
                
                // Test 3: bottom: 0; right: 0 (corner)
                h(TestBox, { title: '3. bottom: 0; right: 0' },
                    h(PositionedElement, {
                        style: { bottom: '0', right: '0', width: '80px', height: '50px' },
                        color: 'rgba(155, 89, 182, 0.8)',
                        label: 'corner'
                    })
                )
            ),
            
            // Row 2: Stretch behaviors
            h('div', { style: { marginBottom: '20px' } },
                h('h2', { style: { fontSize: '16px', margin: '0 0 10px 0' } }, 
                    'Stretch Behaviors (Req 4.3, 4.4)'),
                
                // Test 4: top + bottom stretch (Req 4.3)
                h(TestBox, { title: '4. top: 30; bottom: 30 (v-stretch)' },
                    h(PositionedElement, {
                        style: { top: '30px', bottom: '30px', left: '50px', width: '80px' },
                        color: 'rgba(241, 196, 15, 0.8)',
                        label: 'v-stretch'
                    })
                ),
                
                // Test 5: left + right stretch (Req 4.4)
                h(TestBox, { title: '5. left: 30; right: 30 (h-stretch)' },
                    h(PositionedElement, {
                        style: { left: '30px', right: '30px', top: '50px', height: '50px' },
                        color: 'rgba(52, 73, 94, 0.8)',
                        label: 'h-stretch'
                    })
                ),
                
                // Test 6: All four insets (full stretch)
                h(TestBox, { title: '6. all insets: 30px (full stretch)' },
                    h(PositionedElement, {
                        style: { top: '30px', right: '30px', bottom: '30px', left: '30px' },
                        color: 'rgba(26, 188, 156, 0.8)',
                        label: 'full stretch'
                    })
                )
            ),
            
            // Row 3: Pixel offsets (Req 4.5)
            h('div', { style: { marginBottom: '20px' } },
                h('h2', { style: { fontSize: '16px', margin: '0 0 10px 0' } }, 
                    'Pixel Value Offsets (Req 4.5)'),
                
                // Test 7: bottom: 20px
                h(TestBox, { title: '7. bottom: 20px' },
                    h(PositionedElement, {
                        style: { bottom: '20px', left: '50px', width: '80px', height: '50px' },
                        color: 'rgba(230, 126, 34, 0.8)',
                        label: 'bottom: 20px'
                    })
                ),
                
                // Test 8: right: 40px
                h(TestBox, { title: '8. right: 40px' },
                    h(PositionedElement, {
                        style: { right: '40px', top: '50px', width: '80px', height: '50px' },
                        color: 'rgba(142, 68, 173, 0.8)',
                        label: 'right: 40px'
                    })
                ),
                
                // Test 9: bottom: 30px; right: 30px
                h(TestBox, { title: '9. bottom: 30px; right: 30px' },
                    h(PositionedElement, {
                        style: { bottom: '30px', right: '30px', width: '80px', height: '50px' },
                        color: 'rgba(41, 128, 185, 0.8)',
                        label: 'offset corner'
                    })
                )
            ),
            
            // Row 4: Percentage insets
            h('div', { style: { marginBottom: '20px' } },
                h('h2', { style: { fontSize: '16px', margin: '0 0 10px 0' } }, 
                    'Percentage Insets'),
                
                // Test 10: bottom: 10%
                h(TestBox, { title: '10. bottom: 10%' },
                    h(PositionedElement, {
                        style: { bottom: '10%', left: '50px', width: '80px', height: '50px' },
                        color: 'rgba(192, 57, 43, 0.8)',
                        label: 'bottom: 10%'
                    })
                ),
                
                // Test 11: right: 15%
                h(TestBox, { title: '11. right: 15%' },
                    h(PositionedElement, {
                        style: { right: '15%', top: '50px', width: '80px', height: '50px' },
                        color: 'rgba(39, 174, 96, 0.8)',
                        label: 'right: 15%'
                    })
                ),
                
                // Test 12: all percentage insets
                h(TestBox, { title: '12. all: 15% (% stretch)' },
                    h(PositionedElement, {
                        style: { top: '15%', right: '15%', bottom: '15%', left: '15%' },
                        color: 'rgba(127, 140, 141, 0.8)',
                        label: '% stretch'
                    })
                )
            ),
            
            // Row 5: Viewport units with absolute positioning
            h('div', { style: { marginBottom: '20px' } },
                h('h2', { style: { fontSize: '16px', margin: '0 0 10px 0' } }, 
                    'Viewport Units + Absolute Positioning'),
                
                // Test 13: viewport-relative container with absolute child
                h('div', { 
                    style: {
                        position: 'relative',
                        width: '50vw',
                        height: '20vh',
                        backgroundColor: '#f5f5f5',
                        border: '2px solid #333',
                        margin: '10px',
                        display: 'inline-block'
                    }
                },
                    h('div', { style: titleStyle }, '13. 50vw x 20vh container'),
                    h(PositionedElement, {
                        style: { bottom: '0', right: '0', width: '100px', height: '40px' },
                        color: 'rgba(44, 62, 80, 0.8)',
                        label: 'bottom-right'
                    }),
                    h(PositionedElement, {
                        style: { top: '30px', left: '30px', right: '30px', height: '30px' },
                        color: 'rgba(22, 160, 133, 0.8)',
                        label: 'h-stretch in vh container'
                    })
                )
            ),
            
            // Info panel
            h('div', { 
                style: { 
                    position: 'fixed',
                    bottom: '20px',
                    right: '20px',
                    padding: '15px',
                    backgroundColor: 'white',
                    border: '1px solid #ccc',
                    borderRadius: '5px',
                    fontSize: '12px',
                    maxWidth: '250px',
                    boxShadow: '0 2px 10px rgba(0,0,0,0.1)'
                } 
            }, 
                h('h3', { style: { margin: '0 0 10px 0', fontSize: '14px' } }, 
                    'Test Summary'),
                h('p', { style: { margin: '3px 0' } }, '1-3: Basic single insets'),
                h('p', { style: { margin: '3px 0' } }, '4-6: Stretch behaviors'),
                h('p', { style: { margin: '3px 0' } }, '7-9: Pixel offsets'),
                h('p', { style: { margin: '3px 0' } }, '10-12: Percentage insets'),
                h('p', { style: { margin: '3px 0' } }, '13: Viewport units combo'),
                h('hr', { style: { margin: '10px 0' } }),
                h('p', { style: { margin: '3px 0', color: '#666' } }, 
                    'Requirements: 4.1, 4.2, 4.3, 4.4, 4.5')
            )
        );
    }

    console.log('=== Comprehensive Absolute Positioning Test ===');
    console.log('Testing all inset combinations...');
    render(h(App, null), document.body);
    console.log('Test rendered successfully!');
    console.log('');
    console.log('Expected behaviors:');
    console.log('  Tests 1-3: Elements positioned at edges/corners');
    console.log('  Tests 4-6: Elements stretch to fill space between insets');
    console.log('  Tests 7-9: Elements offset by pixel values from edges');
    console.log('  Tests 10-12: Elements positioned/stretched using percentages');
    console.log('  Test 13: Absolute positioning works in viewport-sized containers');
})();
