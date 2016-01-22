var solution = new Solution('Exercise11');
var project = new Project('Exercise11');

project.addFile('Sources/**');
project.setDebugDir('Deployment');

project.addSubProject(Solution.createProject('Kore'));

solution.addProject(project)

return solution;
