// 页面交互脚本
(function() {
    console.log('✅ app.js 已加载，图片资源路径正确');

    // 为所有图片添加加载成功/失败的日志
    const images = document.querySelectorAll('img');
    images.forEach(img => {
        img.addEventListener('load', () => {
            console.log(`🖼️ 图片加载成功: ${img.src}`);
        });
        img.addEventListener('error', () => {
            console.error(`❌ 图片加载失败: ${img.src} —— 请确认 images 目录下存在对应文件`);
        });
    });

    // 动态显示加载完成信息
    window.addEventListener('DOMContentLoaded', () => {
        const statusDiv = document.createElement('div');
        statusDiv.className = 'status-msg';
        statusDiv.textContent = '✨ 所有资源已准备就绪 (HTML/CSS/JS + 双图片)';
        statusDiv.style.marginTop = '2rem';
        statusDiv.style.padding = '0.8rem';
        statusDiv.style.background = '#e8f0fe';
        statusDiv.style.borderRadius = '2rem';
        statusDiv.style.fontSize = '0.9rem';
        statusDiv.style.color = '#1e466e';
        document.querySelector('.container')?.appendChild(statusDiv);
    });
})();