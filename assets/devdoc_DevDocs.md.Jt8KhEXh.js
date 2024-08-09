import{_ as e,c as t,o as a,V as o}from"./chunks/framework.gBlNPWt_.js";const g=JSON.parse('{"title":"Working with developer documentation","description":"","frontmatter":{},"headers":[],"relativePath":"devdoc/DevDocs.md","filePath":"devdoc/DevDocs.md","lastUpdated":1723205458000}'),i={name:"devdoc/DevDocs.md"},n=o(`<h1 id="working-with-developer-documentation" tabindex="-1">Working with developer documentation <a class="header-anchor" href="#working-with-developer-documentation" aria-label="Permalink to &quot;Working with developer documentation&quot;">​</a></h1><p><em>Some basic guidlines to ensure your documentation works well with VitePress</em></p><h2 id="documentation-location" tabindex="-1">Documentation location <a class="header-anchor" href="#documentation-location" aria-label="Permalink to &quot;Documentation location&quot;">​</a></h2><p>Documents can be located anywhere in the repository folder structure. If it makes sense to have documentation &quot;close&quot; to specific components, then it can be located in the same folder as the component. For example, any developer documentation for specific plugins can be located in those folders. If this isn&#39;t appropriate then the documentation can be located in the <code>devdoc</code> or subfolders of <code>devdoc</code>.</p><div class="warning custom-block"><p class="custom-block-title">WARNING</p><p>There is an exclusion list in the <code>devdoc/.vitepress/config.js</code> file that prevents certain folders from being included in the documentation. If you add a new document to a folder that is excluded, then it will not be included in the documentation. If you need to add a new document to an excluded folder, then you will need to update the exclusion list in the <code>devdoc/.vitepress/config.js</code> file.</p></div><h2 id="documentation-format" tabindex="-1">Documentation format <a class="header-anchor" href="#documentation-format" aria-label="Permalink to &quot;Documentation format&quot;">​</a></h2><p>Documentation is written in <a href="https://www.markdownguide.org/" target="_blank" rel="noreferrer">Markdown</a>. This is a simple format that is easy to read and write. It is also easy to convert to other formats, such as HTML, PDF, and Word. Markdown is supported by many editors, including Visual Studio Code, and is supported by VitePress.</p><div class="tip custom-block"><p class="custom-block-title">TIP</p><p>VitePress extends Markdown with some additional features, such as <a href="https://vitepress.vuejs.org/guide/markdown.html#custom-containers" target="_blank" rel="noreferrer">custom containers</a>, it is recommended that you refer to the <a href="https://vitepress.dev/" target="_blank" rel="noreferrer">VitePress documentation</a> for more details.</p></div><h2 id="rendering-documentation-locally-with-vitepress" tabindex="-1">Rendering documentation locally with VitePress <a class="header-anchor" href="#rendering-documentation-locally-with-vitepress" aria-label="Permalink to &quot;Rendering documentation locally with VitePress&quot;">​</a></h2><p><em>To assist with the writing of documentation, VitePress can be used to render the documentation locally. This allows you to see how the documentation will look when it is published. To start the local development server you need to type the following commands in the root HPCC-Platform folder:</em></p><div class="language-sh vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">sh</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">npm</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> install</span></span>
<span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">npm</span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;"> run docs-dev</span></span></code></pre></div><p>This will start a local development server and display the URL that you can use to view the documentation. The default URL is <a href="http://localhost:5173/HPCC-Platform" target="_blank" rel="noreferrer">http://localhost:5173/HPCC-Platform</a>, but it may be different on your machine. The server will automatically reload when you make changes to the documentation.</p><div class="warning custom-block"><p class="custom-block-title">WARNING</p><p>The first time you start the VitePress server it will take a while to complete. This is because it is locating all the markdown files in the repository and creating the html pages. Once it has completed this step once, it will be much faster to start the server again.</p></div><h2 id="adding-a-new-document" tabindex="-1">Adding a new document <a class="header-anchor" href="#adding-a-new-document" aria-label="Permalink to &quot;Adding a new document&quot;">​</a></h2><p>To add a new document, you need to add a new markdown file to the repository. The file should be named appropriately and have the <code>.md</code> file extension. Once the file exists, you can view it by navigating to the appropriate URL. For example, if you add a new file called <code>MyNewDocument.md</code> to the <code>devdoc</code> folder, then you can view it by navigating to <a href="http://localhost:5173/HPCC-Platform/devdoc/MyNewDocument.html" target="_blank" rel="noreferrer">http://localhost:5173/HPCC-Platform/devdoc/MyNewDocument.html</a>.</p><h2 id="adding-a-new-document-to-the-sidebar" tabindex="-1">Adding a new document to the sidebar <a class="header-anchor" href="#adding-a-new-document-to-the-sidebar" aria-label="Permalink to &quot;Adding a new document to the sidebar&quot;">​</a></h2><p>To add a new document to the sidebar, you need to add an entry to the <code>devdoc/.vitepress/config.js</code> file. The entry should be added to the <code>sidebar</code> section. For example, to add a new document called <code>MyNewDocument.md</code> to the <code>devdoc</code> folder, you would add the following entry to the <code>sidebar</code> section:</p><div class="language-js vp-adaptive-theme"><button title="Copy Code" class="copy"></button><span class="lang">js</span><pre class="shiki shiki-themes github-light github-dark vp-code"><code><span class="line"><span style="--shiki-light:#6F42C1;--shiki-dark:#B392F0;">sidebar</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">: [</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    ...</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    {</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        text: </span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">&#39;My New Document&#39;</span><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">,</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">        link: </span><span style="--shiki-light:#032F62;--shiki-dark:#9ECBFF;">&#39;/devdoc/MyNewDocument&#39;</span></span>
<span class="line"><span style="--shiki-light:#24292E;--shiki-dark:#E1E4E8;">    }</span></span>
<span class="line"><span style="--shiki-light:#D73A49;--shiki-dark:#F97583;">    ...</span></span></code></pre></div><div class="tip custom-block"><p class="custom-block-title">TIP</p><p>You can find more information on the config.js file in the <a href="https://vitepress.dev/reference/site-config" target="_blank" rel="noreferrer">VitePress documentation</a>.</p></div><h2 id="editing-the-main-landing-page" tabindex="-1">Editing the main landing page <a class="header-anchor" href="#editing-the-main-landing-page" aria-label="Permalink to &quot;Editing the main landing page&quot;">​</a></h2><p>The conent of the main landing page is located in <code>index.md</code> in the root folder. Its structure uses the <a href="https://vitepress.dev/reference/default-theme-home-page" target="_blank" rel="noreferrer">VitePress &quot;home&quot; layout</a>.</p>`,21),s=[n];function d(l,r,c,h,p,u){return a(),t("div",null,s)}const f=e(i,[["render",d]]);export{g as __pageData,f as default};
