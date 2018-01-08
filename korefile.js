let project = new Project('Solution11', __dirname);

project.addFile('Sources/**');
project.setDebugDir('Deployment');

Project.createProject('Kore', __dirname).then((kore) => {
	project.addSubProject(kore);
	resolve(project);
});