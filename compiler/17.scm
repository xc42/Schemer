(begin (let ((f2807 (box -1))) (let ((a2808 -1)) (let ((b2809 -1)) (let ((g2810 -1)) (let ((c2811 -1)) (let ((m2812 -1)) (let ((z2813 -1)) (begin (set! z2813 (lambda (x2821) x2821)) (set! m2812 10) (set! c2811 (let ((x 10)) ((let ((zero?2817 (box -1))) (let ((f2818 (box -1))) (begin (set-box! f2818 (lambda (n2820) (if ((unbox zero?2817) n2820) 1 (* n2820 ((unbox f2818) (- n2820 1)))))) (set-box! zero?2817 (lambda (n2819) (= n2819 0))) (unbox f2818)))) x))) (set! g2810 (lambda (h2816) ((unbox f2807) h2816 5))) (set! b2809 (+ 5 7)) (set! a2808 5) (set-box! f2807 (lambda (g2814 x2815) (g2814 x2815))) (begin (set! z2813 (lambda (x2822) (+ x2822 x2822))) (set! m2812 (+ m2812 m2812)) (+ (+ (+ ((unbox f2807) z2813 a2808) ((unbox f2807) z2813 b2809)) ((unbox f2807) z2813 c2811)) (g2810 z2813))))))))))))
